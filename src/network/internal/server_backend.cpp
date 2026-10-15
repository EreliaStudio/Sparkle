#include "network/internal/server_backend.hpp"

#include "exception.hpp"
#include "network/internal/connection_id.hpp"
#include "network/internal/exception_reporting.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/incoming_frame_validator.hpp"
#include "network/internal/page.hpp"
#include "network/internal/server_send_admission.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace spk::NetworkInternal
{
	ServerBackend::Session::Session(ConnectionID id, Socket socket, PageFailureLimits limits, std::chrono::milliseconds timeout, std::shared_ptr<ReassemblyBudget> sharedBudget, std::uint32_t maximumPageBytes) :
		id(id),
		socket(std::move(socket)),
		reception(std::move(sharedBudget)),
		handshake(timeout, maximumPageBytes)
	{
		reception.configureFailureLimits(limits);
	}

	ServerBackend::ServerBackend(ServerCallbacks callbacks) :
		_callbacks(std::move(callbacks))
	{
	}

	void ServerBackend::_unregisterSocket(const Socket &) noexcept
	{
	}

	std::shared_ptr<ServerBackend::Session> ServerBackend::_makeSession(Socket socket)
	{
		const ConnectionID id = allocateConnectionID();
		auto session = std::make_shared<Session>(id, std::move(socket), _failureLimits, _handshakeTimeout, _incomingBudget, static_cast<std::uint32_t>(_maxPageBytes.load()));
		session->reception.setInactivityTimeout(_incomingTransferTimeout);
		session->reception.setLimits(_maxMessageBytes.load(), _maxIncomingBytes.load());
		session->transfers.setMaxMessageBytes(_maxMessageBytes.load());
		session->transfers.setAcknowledgmentTimeout(_acknowledgmentTimeout);
		return session;
	}

	void ServerBackend::_registerSession(const std::shared_ptr<Session> &session)
	{
		const std::scoped_lock lock(_commandMutex);
		_sessions.emplace(session->id, session);
		++_sessionCount;
	}

	void ServerBackend::_notifyConnection(ConnectionID id) noexcept
	{
		try
		{
			_callbacks.onConnection(id);
		} catch (...)
		{
			NetworkInternal::reportCurrentException("Network server Pending callback failed.");
		}
	}

	void ServerBackend::_notifyReady(ConnectionID id) noexcept
	{
		try
		{
			_callbacks.onReady(id);
		} catch (...)
		{
			NetworkInternal::reportCurrentException("Network server Connected callback failed.");
		}
	}

	void ServerBackend::_notifyDisconnection(ConnectionID id, Connection::Error error) noexcept
	{
		try
		{
			_callbacks.onDisconnection(id, error);
		} catch (...)
		{
			NetworkInternal::reportCurrentException("Network server Disconnected callback failed.");
		}
	}

	void ServerBackend::_publish(ReceivedMessage message) noexcept
	{
		try
		{
			_callbacks.onMessage(std::move(message));
		} catch (...)
		{
			NetworkInternal::reportCurrentException("Network server receive publication failed.");
		}
	}

	void ServerBackend::_appendReceived(Session &session, const std::byte *data, std::size_t size)
	{
		session.receivedBytes.insert(session.receivedBytes.end(), data, data + size);
		std::size_t consumed = 0;
		while (_sessions.contains(session.id) && _extractMessage(session, consumed))
		{
		}
		if (consumed != 0)
		{
			session.receivedBytes.erase(
				session.receivedBytes.begin(),
				session.receivedBytes.begin() + static_cast<std::ptrdiff_t>(consumed));
		}
	}

	void ServerBackend::_queueHandshake(Session &session, Handshake::Kind kind, Connection::Error error)
	{
		auto frame = Handshake::encode(kind, session.id, error, session.handshake.maximumPageBytes());
		auto reservation = _commandBudget.reserve(frame->size(), CommandBudget::Stage::Frame);
		const std::size_t queuedBytes = frame->size();
		session.outgoing.push_back(TransmissionFrame{std::move(frame), 0, static_cast<std::uint16_t>(kind), std::move(reservation)});
		session.queuedBytes += queuedBytes;
		_requestWrite(session);
	}

	void ServerBackend::_handleHello(Session &session, std::span<const std::byte> bytes)
	{
		const auto kind = session.handshake.accept(bytes);
		_queueHandshake(session, kind, session.handshake.error());
	}

	void ServerBackend::_handshakeWritten(Session &session, std::uint16_t kind)
	{
		if (session.handshake.written(static_cast<Handshake::Kind>(kind)))
		{
			{
				const std::scoped_lock lock(_commandMutex);
				++_readySessions;
			}
			_notifyReady(session.id);
		}
		else
		{
			_disconnect(session.id);
		}
	}

	void ServerBackend::_queueResponse(Session &session, const TransferResponse &response)
	{
		auto frame = TransferResponse::Codec::encode(response);
		auto reservation = _commandBudget.reserve(frame->size(), CommandBudget::Stage::Frame);
		const std::size_t queuedBytes = frame->size();
		session.outgoing.push_back(TransmissionFrame{std::move(frame), 0, 0, std::move(reservation)});
		session.queuedBytes += queuedBytes;
		_requestWrite(session);
	}

	bool ServerBackend::_extractMessage(Session &session, std::size_t &consumed)
	{
		const std::size_t remaining = session.receivedBytes.size() - consumed;
		if (remaining < FrameHeaderSize)
		{
			return false;
		}
		const auto *data = session.receivedBytes.data() + consumed;
		const auto header = decode(std::span<const std::byte, FrameHeaderSize>(data, FrameHeaderSize));
		const std::size_t frameSize = FrameHeaderSize + header.payloadSize;
		const auto kind = read16(data + 6);
		if (session.handshake.ready())
		{
			IncomingFrameValidator::validate(
				kind,
				header.payloadSize,
				session.handshake.maximumPageBytes(),
				_maxMessageBytes.load());
		}
		if (!session.handshake.ready() && kind != static_cast<std::uint16_t>(Handshake::Kind::Hello))
		{
			throw Exception("Application traffic is forbidden during handshake.");
		}
		if (!session.handshake.ready() && header.payloadSize != Handshake::PayloadSize)
		{
			throw Exception("Invalid initial handshake payload size.");
		}
		if (header.connectionID != InvalidConnectionID && header.connectionID != session.id)
		{
			throw Exception("Incoming message contains a forged connection ID.");
		}
		if (remaining < frameSize)
		{
			return false;
		}
		if (!session.handshake.ready())
		{
			_handleHello(session, std::span(data, frameSize));
		}
		else if (kind == PageFrameKind)
		{
			const auto page = decodePage(std::span(data, frameSize));
			auto received = session.reception.accept(page, session.id);
			if (received.message.has_value())
			{
				_publish(ReceivedMessage{session.id, received.message->withConnectionID(session.id)});
			}
			if (received.response.has_value())
			{
				_queueResponse(session, *received.response);
			}
		}
		else if (kind == TransferResponse::Codec::FrameKind)
		{
			const auto response = TransferResponse::Codec::decode(std::span(data, frameSize));
			if (response.connectionID != session.id)
			{
				throw Exception("Transfer response has an incorrect connection ID.");
			}
			(void)session.transfers.acknowledge(response);
			_releaseReservation(session, response.transferID);
		}
		else if (kind == 0)
		{
			Message::Writer writer(header.type);
			writer.setRequestID(header.requestID);
			writer.setConnectionID(session.id);
			writer.resize(header.payloadSize);
			std::copy_n(data + FrameHeaderSize, header.payloadSize, writer.data().begin());
			_publish(ReceivedMessage{session.id, std::move(writer).build()});
			session.reception.successfulMessage();
		}
		else
		{
			throw Exception("Unknown network frame kind.");
		}
		consumed += frameSize;
		return true;
	}

	void ServerBackend::_releaseReservation(Session &session, spk::Transfer::ID id) noexcept
	{
		const auto it = session.outgoingReservations.find(id);
		if (it == session.outgoingReservations.end())
		{
			return;
		}
		session.reservedOutgoingBytes -= it->second.bytes();
		session.outgoingReservations.erase(it);
	}

	void ServerBackend::_queueMessage(const std::shared_ptr<Session> &session, const Message &message, Message::Priority priority, ServerSendBatch::Recipient &recipient)
	{
		if (!session->handshake.ready())
		{
			throw Exception("Cannot send to a pending network connection.");
		}
		auto tracked = session->transfers.enqueue(message.withConnectionID(session->id), priority, recipient.transfer);
		try
		{
			const std::size_t bytes = recipient.reservation.bytes();
			session->outgoingReservations.emplace(tracked.id(), std::move(recipient.reservation));
			session->reservedOutgoingBytes += bytes;
		} catch (...)
		{
			(void)session->transfers.fail(tracked.id(), spk::Transfer::Status::Rejected, spk::Transfer::Reason::QueueOverflow);
			throw;
		}
	}

	void ServerBackend::_disconnect(ConnectionID id) noexcept
	{
		const auto iterator = _sessions.find(id);
		if (iterator == _sessions.end())
		{
			return;
		}

		auto session = std::move(iterator->second);
		{
			const std::scoped_lock lock(_commandMutex);
			_sessions.erase(iterator);
			--_sessionCount;
			if (session->handshake.ready())
			{
				--_readySessions;
			}
		}
		_unregisterSocket(session->socket);
		session->socket.close();
		session->transfers.clear();
		session->reception.clear();
		_notifyDisconnection(id, session->handshake.error());
	}

	void ServerBackend::_disconnectAll() noexcept
	{
		std::vector<ConnectionID> ids;
		ids.reserve(_sessions.size());
		for (const auto &entry : _sessions)
		{
			ids.push_back(entry.first);
		}
		for (const ConnectionID id : ids)
		{
			_disconnect(id);
		}
	}

	void ServerBackend::_enqueue(Command command)
	{
		{
			const std::scoped_lock lock(_commandMutex);
			if (command.batch != nullptr && !_running.load())
			{
				throw Exception("Unable to admit sends on a stopped network server.");
			}
			if (command.batch != nullptr)
			{
				// Linearize an empty broadcast with backend readiness, before reserving
				// or retaining any command payload. Only SkipRecipient excludes Pending peers.
				if (command.batch->allRecipients && (_sessionCount == 0 || (command.batch->policy == Connection::OverflowPolicy::SkipRecipient && _readySessions == 0)))
				{
					command.batch->acceptAdmission();
					return;
				}
				const std::size_t bytes = std::max<std::size_t>(1, command.batch->message.size());
				command.reservation = _commandBudget.reserve(bytes, CommandBudget::Stage::Command);
			}
			_commands.push_back(std::move(command));
		}
		_wake();
	}

	std::deque<ServerBackend::Command> ServerBackend::_takeCommands()
	{
		const std::scoped_lock lock(_commandMutex);
		std::deque<Command> result;
		result.swap(_commands);
		return result;
	}

	void ServerBackend::_processCommands()
	{
		auto commands = _takeCommands();
		try
		{
			for (Command &command : commands)
			{
				if (_stopping)
				{
					if (command.batch != nullptr)
					{
						command.batch->cancelAdmission();
					}
					continue;
				}
				_processCommand(std::move(command));
			}
		} catch (...)
		{
			for (Command &command : commands)
			{
				if (command.batch != nullptr)
				{
					command.batch->cancelAdmission();
				}
			}
			throw;
		}
	}

	void ServerBackend::_processCommand(Command command)
	{
		if (command.kind == CommandKind::Stop)
		{
			_beginStop();
			return;
		}
		if (command.kind == CommandKind::ConfigureLimits)
		{
			_failureLimits = command.limits;
			for (auto &[id, session] : _sessions)
			{
				(void)id;
				session->reception.configureFailureLimits(_failureLimits);
			}
			return;
		}
		if (command.kind == CommandKind::ConfigureHandshakeTimeout)
		{
			_handshakeTimeout = command.handshakeTimeout;
			return;
		}
		if (command.kind == CommandKind::ConfigureIncomingTransferTimeout)
		{
			_incomingTransferTimeout = command.transferTimeout;
			for (auto &[id, session] : _sessions)
			{
				(void)id;
				session->reception.setInactivityTimeout(_incomingTransferTimeout);
			}
			return;
		}
		if (command.kind == CommandKind::ConfigureAcknowledgmentTimeout)
		{
			_acknowledgmentTimeout = command.transferTimeout;
			for (auto &[id, session] : _sessions)
			{
				(void)id;
				session->transfers.setAcknowledgmentTimeout(_acknowledgmentTimeout);
			}
			return;
		}
		if (command.kind == CommandKind::ConfigureIncomingLimits)
		{
			_maxMessageBytes.store(command.maxMessageBytes);
			_maxIncomingBytes.store(command.maxIncomingBytes);
			for (auto &[id, session] : _sessions)
			{
				(void)id;
				session->reception.setLimits(command.maxMessageBytes, command.maxIncomingBytes);
				session->transfers.setMaxMessageBytes(command.maxMessageBytes);
			}
			return;
		}
		command.reservation.reset();
		try
		{
			_sendBatch(*command.batch);
			command.batch->acceptAdmission();
		} catch (...)
		{
			command.batch->rejectAdmission(std::current_exception());
		}
	}

	void ServerBackend::_sendBatch(ServerSendBatch &batch)
	{
		ServerSendAdmission<Session> admission(batch, _sessions, _commandBudget, _callbacks.snapshotConnections);
		admission.run(
			[this, &batch](const std::shared_ptr<Session> &session, ServerSendBatch::Recipient &recipient) {
				_queueMessage(session, batch.message, batch.priority, recipient);
			},
			[this](const std::shared_ptr<Session> &session) {
				_writeSession(session);
			},
			[this](ConnectionID id) {
				_disconnect(id);
			});
	}

	void ServerBackend::_beginStop() noexcept
	{
		_stopping = true;
		_running = false;
		_unregisterSocket(_listener);
		_listener.close();
		_disconnectAll();
		std::deque<Command> abandoned;
		{
			const std::scoped_lock lock(_commandMutex);
			abandoned.swap(_commands);
		}
		for (Command &command : abandoned)
		{
			if (command.batch != nullptr)
			{
				command.batch->cancelAdmission();
			}
		}
	}

	void ServerBackend::_expireReassemblies()
	{
		std::vector<ConnectionID> ids;
		ids.reserve(_sessions.size());
		for (const auto &[id, session] : _sessions)
		{
			ids.push_back(id);
		}
		for (const auto id : ids)
		{
			const auto it = _sessions.find(id);
			if (it == _sessions.end())
			{
				continue;
			}
			auto session = it->second;
			if (!session->handshake.ready())
			{
				if (session->handshake.expired())
				{
					session->handshake.fail(Connection::Error::Timeout);
					_disconnect(id);
				}
				continue;
			}
			try
			{
				for (const auto &response : session->reception.expire(id))
				{
					if (!_sessions.contains(id))
					{
						break;
					}
					_queueResponse(*session, response);
				}
				if (_sessions.contains(id))
				{
					session->transfers.expire();
					for (auto reservation = session->outgoingReservations.begin(); reservation != session->outgoingReservations.end();)
					{
						const auto id = reservation->first;
						++reservation;
						if (!session->transfers.isTracked(id))
						{
							_releaseReservation(*session, id);
						}
					}
				}
			} catch (...)
			{
				reportCurrentException("Network server transfer expiration failed.");
				_disconnect(id);
			}
		}
	}

	bool ServerBackend::isRunning() const noexcept
	{
		return _running.load();
	}

	std::uint16_t ServerBackend::port() const noexcept
	{
		return _port.load();
	}

	bool ServerBackend::isWorkerThread() const noexcept
	{
		return _worker.joinable() && _worker.get_id() == std::this_thread::get_id();
	}

	void ServerBackend::start(std::uint16_t port)
	{
		if (_running.load())
		{
			throw Exception("Stop the network server before starting it again.");
		}
		stop();
		{
			const std::scoped_lock lock(_commandMutex);
			_commands.clear();
		}
		_stopping = false;
		try
		{
			_openEvents();
			_listener = Socket::listenTCP(port);
			_port = _listener.localPort();
			_listen();
		} catch (...)
		{
			_listener.close();
			_closeEvents();
			throw;
		}
		_running = true;
		_worker = std::jthread([this] {
			_run();
		});
	}

	void ServerBackend::stop()
	{
		if (_worker.joinable() &&
			_worker.get_id() == std::this_thread::get_id())
		{
			_beginStop();
			return;
		}
		if (_worker.joinable())
		{
			if (_running)
			{
				_enqueue(Command{CommandKind::Stop});
			}
			else
			{
				_wake();
			}
			_worker.join();
		}
		_listener.close();
		_disconnectAll();
		_closeEvents();
		_port = 0;
		_running = false;
		_stopping = true;
	}

	void ServerBackend::setPageFailureLimits(PageFailureLimits limits)
	{
		if (!limits.valid())
		{
			throw Exception("Network page failure limits must be positive.");
		}
		if (_running)
		{
			Command command{CommandKind::ConfigureLimits};
			command.limits = limits;
			_enqueue(std::move(command));
		}
		else
		{
			_failureLimits = limits;
		}
	}

	void ServerBackend::setHandshakeTimeout(std::chrono::milliseconds timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Handshake timeout must be positive.");
		}
		if (!_running.load())
		{
			_handshakeTimeout = timeout;
			return;
		}
		Command command{};
		command.kind = CommandKind::ConfigureHandshakeTimeout;
		command.handshakeTimeout = timeout;
		_enqueue(std::move(command));
	}

	void ServerBackend::setIncomingTransferTimeout(std::chrono::milliseconds timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Incoming transfer inactivity timeout must be positive.");
		}
		if (!_running.load())
		{
			_incomingTransferTimeout = timeout;
			return;
		}
		Command command{};
		command.kind = CommandKind::ConfigureIncomingTransferTimeout;
		command.transferTimeout = timeout;
		_enqueue(std::move(command));
	}

	void ServerBackend::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Acknowledgment timeout must be positive.");
		}
		if (!_running.load())
		{
			_acknowledgmentTimeout = timeout;
			return;
		}
		Command command{};
		command.kind = CommandKind::ConfigureAcknowledgmentTimeout;
		command.transferTimeout = timeout;
		_enqueue(std::move(command));
	}

	void ServerBackend::setGlobalMaxQueuedBytes(std::size_t bytes)
	{
		if (bytes == 0)
		{
			throw Exception("Global outgoing network memory limit must be positive.");
		}
		_commandBudget.setMaximum(bytes);
	}

	void ServerBackend::_configureIncomingLimits(std::size_t messageBytes, std::size_t incomingBytes)
	{
		_maxMessageBytes.store(messageBytes);
		_maxIncomingBytes.store(incomingBytes);
		if (!_running.load())
		{
			return;
		}
		Command command{};
		command.kind = CommandKind::ConfigureIncomingLimits;
		command.maxMessageBytes = messageBytes;
		command.maxIncomingBytes = incomingBytes;
		_enqueue(std::move(command));
	}

	void ServerBackend::setMaxMessageBytes(std::size_t bytes)
	{
		if (bytes == 0)
		{
			throw Exception("Maximum logical message size must be positive.");
		}
		_configureIncomingLimits(bytes, _maxIncomingBytes.load());
	}

	void ServerBackend::setMaxIncomingBytes(std::size_t bytes)
	{
		if (bytes == 0)
		{
			throw Exception("Incoming reassembly budget must be positive.");
		}
		_configureIncomingLimits(_maxMessageBytes.load(), bytes);
	}

	void ServerBackend::setMaxPageBytes(std::size_t bytes)
	{
		if (bytes == 0 || bytes > MaximumSupportedPageBytes)
		{
			throw Exception("Network page size must be within the supported protocol limit.");
		}
		_maxPageBytes.store(bytes);
	}

	void ServerBackend::setGlobalMaxIncomingBytes(std::size_t bytes)
	{
		_incomingBudget->setMaximum(bytes);
	}

	void ServerBackend::send(std::shared_ptr<ServerSendBatch> batch)
	{
		if (!_running)
		{
			throw Exception("Unable to send through a stopped network server.");
		}
		try
		{
			if (_worker.get_id() == std::this_thread::get_id())
			{
				_sendBatch(*batch);
				batch->acceptAdmission();
				return;
			}
			auto admitted = batch->admission.get_future();
			Command command{CommandKind::Send};
			command.batch = std::move(batch);
			_enqueue(std::move(command));
			admitted.get();
		} catch (const Exception &)
		{
			throw;
		} catch (...)
		{
			throw Exception("Network send admission failed.", std::current_exception());
		}
	}

	bool ServerBackend::_prepareWrite(Session &session)
	{
		if (session.outgoing.empty() && !_stopping)
		{
			auto page = session.transfers.nextPage((std::min)(_maxPageBytes.load(), static_cast<std::size_t>(session.handshake.maximumPageBytes())));
			if (page.has_value())
			{
				auto frame = encodePage(*page);
				const auto reservation = session.outgoingReservations.find(page->transferID);
				if (reservation == session.outgoingReservations.end())
				{
					throw Exception("Missing outgoing page reservation.");
				}
				session.queuedBytes += frame->size();
				session.outgoing.push_back(TransmissionFrame{std::move(frame), page->offset + page->length == page->totalSize ? page->transferID : 0, 0, reservation->second.frame, page->transferID});
			}
		}
		if (session.outgoing.empty() || _stopping)
		{
			return false;
		}
		if (session.outgoing.front().reservation != nullptr)
		{
			session.outgoing.front().reservation->transition(CommandBudget::Stage::InFlight);
		}
		return true;
	}

	void ServerBackend::_completeFrame(Session &session, std::size_t bytes)
	{
		session.outgoingOffset += bytes;
		const auto &frame = session.outgoing.front();
		if (session.outgoingOffset != frame.bytes->size())
		{
			return;
		}
		session.queuedBytes -= frame.bytes->size();
		const auto finalPage = frame.finalPage;
		const auto handshakeKind = frame.handshakeKind;
		if (frame.reservation != nullptr)
		{
			frame.reservation->transition(CommandBudget::Stage::Frame);
		}
		session.outgoing.pop_front();
		session.outgoingOffset = 0;
		if (finalPage != 0)
		{
			session.transfers.finalPageWritten(finalPage);
			_releaseReservation(session, finalPage);
		}
		if (handshakeKind != 0)
		{
			_handshakeWritten(session, handshakeKind);
		}
	}
}
