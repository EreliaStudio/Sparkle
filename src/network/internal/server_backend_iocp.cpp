#include "network/internal/server_backend.hpp"

#ifdef _WIN32

#	include "exception.hpp"
#	include "network/internal/command_budget.hpp"
#	include "network/internal/outgoing_reservation.hpp"
#	include "network/internal/connection_id.hpp"
#	include "network/internal/frame.hpp"
#	include "network/internal/handshake.hpp"
#	include "network/internal/server_handshake.hpp"
#	include "network/internal/server_send_admission.hpp"
#	include "network/internal/page.hpp"
#	include "network/internal/transmission_frame.hpp"
#	include "network/internal/transfer_reception.hpp"
#	include "network/internal/transfer_sender.hpp"
#	include "network/internal/socket.hpp"

#	include <algorithm>
#	include <array>
#	include <atomic>
#	include <chrono>
#	include <cstddef>
#	include <cstdint>
#	include <deque>
#	include <exception>
#	include <future>
#	include <memory>
#	include <mutex>
#	include <string>
#	include <thread>
#	include <unordered_map>
#	include <utility>
#	include <vector>

#	include <winsock2.h>

#	include <mswsock.h>
#	include <ws2tcpip.h>

#	include <Windows.h>

namespace spk::NetworkInternal
{
	namespace
	{
		using Frame = std::shared_ptr<std::vector<std::byte>>;
		constexpr ULONG_PTR WakeKey = 1;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t AcceptOperationCount = 4;
		constexpr std::size_t MaximumQueuedBytes = 64u * 1024u * 1024u;

		struct Session
		{
			ConnectionID id;
			Socket socket;
			std::vector<std::byte> receivedBytes;
			TransferReception reception;
			ServerHandshake handshake;
			TransferSender transfers;
			std::deque<TransmissionFrame> outgoing;
			std::size_t outgoingOffset = 0;
			std::size_t queuedBytes = 0;
			std::size_t reservedOutgoingBytes = 0;
			std::unordered_map<spk::Transfer::ID, OutgoingReservation> outgoingReservations;
			bool writeInFlight = false;

			Session(ConnectionID id, Socket socket, PageFailureLimits limits, std::chrono::milliseconds timeout, std::shared_ptr<ReassemblyBudget> sharedBudget, std::uint32_t maximumPageBytes) :
				id(id),
				socket(std::move(socket)),
				reception(std::move(sharedBudget)),
				handshake(timeout, maximumPageBytes)
			{
				reception.configureFailureLimits(limits);
			}
		};

		enum class OperationKind
		{
			Accept,
			Read,
			Write
		};

		struct Operation
		{
			OVERLAPPED overlapped{};
			OperationKind kind;
			std::shared_ptr<Session> session;
			Socket acceptedSocket;
			std::array<std::byte, ReceptionBufferSize> buffer{};
			WSABUF wsabuf{};

			explicit Operation(OperationKind kind) :
				kind(kind)
			{
			}
		};

		enum class CommandKind
		{
			Send,
			ConfigureLimits,
			ConfigureHandshakeTimeout,
			ConfigureIncomingTransferTimeout,
			ConfigureAcknowledgmentTimeout,
			ConfigureIncomingLimits,
			Stop
		};

		struct Command
		{
			CommandKind kind;
			ConnectionID connection = InvalidConnectionID;
			std::shared_ptr<const Message> message;
			Message::Priority priority = Message::Priority::Normal;
			PageFailureLimits limits;
			std::chrono::milliseconds handshakeTimeout = Handshake::DefaultTimeout;
			std::chrono::milliseconds transferTimeout = std::chrono::seconds(60);
			std::shared_ptr<ServerSendBatch> batch;
			std::shared_ptr<CommandBudget::Lease> reservation;
			std::size_t maxMessageBytes = 128u * 1024u * 1024u;
			std::size_t maxIncomingBytes = 512u * 1024u * 1024u;
		};

		class IocpServerBackend final : public ServerBackend
		{
		private:
			ServerCallbacks _callbacks;
			Socket _listener;
			std::jthread _worker;
			std::atomic_bool _running = false;
			std::atomic_uint16_t _port = 0;
			HANDLE _iocp = nullptr;
			LPFN_ACCEPTEX _acceptEx = nullptr;
			std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;
			CommandBudget _commandBudget{256u * 1024u * 1024u};
			std::mutex _commandMutex;
			std::deque<Command> _commands;
			PageFailureLimits _failureLimits;
			std::chrono::milliseconds _handshakeTimeout = Handshake::DefaultTimeout;
			std::chrono::milliseconds _incomingTransferTimeout = std::chrono::seconds(60);
			std::chrono::milliseconds _acknowledgmentTimeout = std::chrono::seconds(30);
			std::atomic_size_t _maxGlobalQueuedBytes = 256u * 1024u * 1024u;
			std::atomic_size_t _maxMessageBytes = 128u * 1024u * 1024u;
			std::atomic_size_t _maxIncomingBytes = 512u * 1024u * 1024u;
			std::atomic_size_t _maxPageBytes = DefaultMaximumPageBytes;
			std::shared_ptr<ReassemblyBudget> _incomingBudget = std::make_shared<ReassemblyBudget>();
			std::size_t _pendingOperations = 0;
			bool _stopping = false;

			void _notifyConnection(ConnectionID id) noexcept
			{
				try
				{
					_callbacks.onConnection(id);
				} catch (...)
				{
				}
			}

			void _notifyReady(ConnectionID id) noexcept
			{
				try
				{
					_callbacks.onReady(id);
				} catch (...)
				{
				}
			}

			void _notifyDisconnection(ConnectionID id, Connection::Error error) noexcept
			{
				try
				{
					_callbacks.onDisconnection(id, error);
				} catch (...)
				{
				}
			}

			void _publish(ReceivedMessage message) noexcept
			{
				try
				{
					_callbacks.onMessage(std::move(message));
				} catch (...)
				{
				}
			}

			void _associate(const Socket &socket)
			{
				const HANDLE result = ::CreateIoCompletionPort(
					reinterpret_cast<HANDLE>(static_cast<SOCKET>(socket.nativeHandle())),
					_iocp,
					0,
					0);
				if (result == nullptr)
				{
					throw Exception("Unable to associate socket with IOCP [" + std::to_string(::GetLastError()) + "].");
				}
			}

			void _loadAcceptEx()
			{
				GUID guid = WSAID_ACCEPTEX;
				DWORD bytes = 0;
				const int result = ::WSAIoctl(
					static_cast<SOCKET>(_listener.nativeHandle()),
					SIO_GET_EXTENSION_FUNCTION_POINTER,
					&guid,
					sizeof(guid),
					&_acceptEx,
					sizeof(_acceptEx),
					&bytes,
					nullptr,
					nullptr);
				if (result == SOCKET_ERROR || _acceptEx == nullptr)
				{
					throw Exception("Unable to load AcceptEx [" + std::to_string(::WSAGetLastError()) + "].");
				}
			}

			void _postAccept()
			{
				auto *operation = new Operation(OperationKind::Accept);
				operation->acceptedSocket = Socket::createTCPv4();
				DWORD bytes = 0;
				const DWORD addressSize = sizeof(sockaddr_storage) + 16;
				const BOOL result = _acceptEx(
					static_cast<SOCKET>(_listener.nativeHandle()),
					static_cast<SOCKET>(operation->acceptedSocket.nativeHandle()),
					operation->buffer.data(),
					0,
					addressSize,
					addressSize,
					&bytes,
					&operation->overlapped);
				if (!result && ::WSAGetLastError() != ERROR_IO_PENDING)
				{
					delete operation;
					throw Exception("Unable to post AcceptEx [" + std::to_string(::WSAGetLastError()) + "].");
				}
				++_pendingOperations;
			}

			void _completeAccept(std::unique_ptr<Operation> operation, bool success)
			{
				if (!success || _stopping)
				{
					if (!_stopping)
					{
						_postAccept();
					}
					return;
				}

				SOCKET listener = static_cast<SOCKET>(_listener.nativeHandle());
				const SOCKET accepted = static_cast<SOCKET>(operation->acceptedSocket.nativeHandle());
				if (::setsockopt(
						accepted,
						SOL_SOCKET,
						SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<const char *>(&listener),
						sizeof(listener)) != 0)
				{
					_postAccept();
					return;
				}

				const ConnectionID id = allocateConnectionID();
				auto session = std::make_shared<Session>(id, std::move(operation->acceptedSocket), _failureLimits, _handshakeTimeout, _incomingBudget, static_cast<std::uint32_t>(_maxPageBytes.load()));
				session->reception.setInactivityTimeout(_incomingTransferTimeout);
				session->reception.setLimits(_maxMessageBytes.load(), _maxIncomingBytes.load());
				session->transfers.setMaxMessageBytes(_maxMessageBytes.load());
				session->transfers.setAcknowledgmentTimeout(_acknowledgmentTimeout);
				_associate(session->socket);
				_sessions.emplace(id, session);
				_postRead(session);
				_notifyConnection(id);
				if (!_stopping)
				{
					_postAccept();
				}
			}

			void _postRead(const std::shared_ptr<Session> &session)
			{
				auto *operation = new Operation(OperationKind::Read);
				operation->session = session;
				operation->wsabuf.buf = reinterpret_cast<char *>(operation->buffer.data());
				operation->wsabuf.len = static_cast<ULONG>(operation->buffer.size());
				DWORD flags = 0;
				DWORD bytes = 0;
				const int result = ::WSARecv(
					static_cast<SOCKET>(session->socket.nativeHandle()),
					&operation->wsabuf,
					1,
					&bytes,
					&flags,
					&operation->overlapped,
					nullptr);
				if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
				{
					delete operation;
					_disconnect(session->id);
					return;
				}
				++_pendingOperations;
			}

			void _completeRead(
				const std::shared_ptr<Session> &session,
				const Operation &operation,
				DWORD bytes,
				bool success)
			{
				if (!success || bytes == 0 || !_sessions.contains(session->id))
				{
					_disconnect(session->id);
					return;
				}

				session->receivedBytes.insert(
					session->receivedBytes.end(),
					operation.buffer.begin(),
					operation.buffer.begin() + static_cast<std::ptrdiff_t>(bytes));
				try
				{
					_extractMessages(*session);
				} catch (...)
				{
					_disconnect(session->id);
					return;
				}
				if (_sessions.contains(session->id) && !_stopping)
				{
					_postRead(session);
				}
			}

			void _extractMessages(Session &session)
			{
				std::size_t consumed = 0;
				while (_extractMessage(session, consumed))
				{
				}
				if (consumed != 0)
				{
					session.receivedBytes.erase(
						session.receivedBytes.begin(),
						session.receivedBytes.begin() + static_cast<std::ptrdiff_t>(consumed));
				}
			}

			void _queueHandshake(Session &session, Handshake::Kind kind, Connection::Error error = Connection::Error::None)
			{
				auto frame = Handshake::encode(kind, session.id, error, session.handshake.maximumPageBytes());
				session.queuedBytes += frame->size();
				auto reservation = _commandBudget.reserve(frame->size(), CommandBudget::Stage::Frame);
				session.outgoing.push_back(TransmissionFrame{std::move(frame), 0, static_cast<std::uint16_t>(kind), std::move(reservation)});
				if (!session.writeInFlight)
				{
					_postWrite(_sessions.at(session.id));
				}
			}

			void _handleHello(Session &session, std::span<const std::byte> bytes)
			{
				const auto kind = session.handshake.accept(bytes);
				_queueHandshake(session, kind, session.handshake.error());
			}

			void _handshakeWritten(Session &session, std::uint16_t kind)
			{
				if (session.handshake.written(static_cast<Handshake::Kind>(kind)))
				{
					_notifyReady(session.id);
				}
				else
				{
					_disconnect(session.id);
				}
			}

			void _queueResponse(Session &session, const TransferResponse &response)
			{
				auto frame = response.encode();
				session.queuedBytes += frame->size();
				auto reservation = _commandBudget.reserve(frame->size(), CommandBudget::Stage::Frame);
				session.outgoing.push_back(TransmissionFrame{std::move(frame), 0, 0, std::move(reservation)});
				if (!session.writeInFlight)
				{
					_postWrite(_sessions.at(session.id));
				}
			}

			[[nodiscard]] bool _extractMessage(Session &session, std::size_t &consumed)
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
				if (kind == 0 && header.payloadSize > _maxMessageBytes.load())
				{
					throw Exception("Incoming logical message exceeds the configured maximum.");
				}
				if (!session.handshake.ready() && kind != static_cast<std::uint16_t>(Handshake::Kind::Hello))
				{
					throw Exception("Application traffic is forbidden during handshake.");
				}
				if (!session.handshake.ready() && header.payloadSize != Handshake::PayloadSize)
				{
					throw Exception("Invalid initial handshake payload size.");
				}
				if (remaining < frameSize)
				{
					return false;
				}
				if (header.connectionID != InvalidConnectionID && header.connectionID != session.id)
				{
					throw Exception("Incoming message contains a forged connection ID.");
				}
				if (!session.handshake.ready())
				{
					_handleHello(session, std::span(data, frameSize));
				}
				else if (kind == PageFrameKind)
				{
					const auto page = decodePage(std::span(data, frameSize));
					if (page.data.size() > session.handshake.maximumPageBytes())
					{
						throw Exception("Network page exceeds the negotiated maximum payload size.");
					}
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
				else if (kind == TransferResponse::FrameKind)
				{
					const auto response = TransferResponse::decode(std::span(data, frameSize));
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

			void _releaseReservation(Session &session, spk::Transfer::ID id) noexcept
			{
				const auto it = session.outgoingReservations.find(id);
				if (it == session.outgoingReservations.end())
				{
					return;
				}
				session.reservedOutgoingBytes -= it->second.bytes();
				session.outgoingReservations.erase(it);
			}

			void _queueMessage(const std::shared_ptr<Session> &session, const Message &message, Message::Priority priority, ServerSendBatch::Recipient &recipient)
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

			void _postWrite(const std::shared_ptr<Session> &session)
			{
				if (session->outgoing.empty() && !_stopping)
				{
					auto page = session->transfers.nextPage((std::min)(_maxPageBytes.load(), static_cast<std::size_t>(session->handshake.maximumPageBytes())));
					if (page.has_value())
					{
						Frame frame = encodePage(*page);
						session->queuedBytes += frame->size();
						const auto reservation = session->outgoingReservations.find(page->transferID);
						if (reservation == session->outgoingReservations.end())
						{
							throw Exception("Missing outgoing page reservation.");
						}
						session->outgoing.push_back(TransmissionFrame{std::move(frame), page->offset + page->length == page->totalSize ? page->transferID : 0, 0, reservation->second.frame, page->transferID});
					}
				}
				if (session->outgoing.empty() || _stopping)
				{
					session->writeInFlight = false;
					return;
				}

				const Frame &frame = session->outgoing.front().bytes;
				if (session->outgoing.front().reservation != nullptr)
				{
					session->outgoing.front().reservation->transition(CommandBudget::Stage::InFlight);
				}
				auto *operation = new Operation(OperationKind::Write);
				operation->session = session;
				operation->wsabuf.buf = reinterpret_cast<char *>(
					frame->data() + session->outgoingOffset);
				operation->wsabuf.len = static_cast<ULONG>(
					frame->size() - session->outgoingOffset);
				DWORD bytes = 0;
				const int result = ::WSASend(
					static_cast<SOCKET>(session->socket.nativeHandle()),
					&operation->wsabuf,
					1,
					&bytes,
					0,
					&operation->overlapped,
					nullptr);
				if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
				{
					delete operation;
					_disconnect(session->id);
					return;
				}
				session->writeInFlight = true;
				++_pendingOperations;
			}

			void _completeWrite(
				const std::shared_ptr<Session> &session,
				DWORD bytes,
				bool success)
			{
				if (!success || bytes == 0 || !_sessions.contains(session->id))
				{
					_disconnect(session->id);
					return;
				}

				session->outgoingOffset += static_cast<std::size_t>(bytes);
				const Frame &frame = session->outgoing.front().bytes;
				if (session->outgoingOffset == frame->size())
				{
					session->queuedBytes -= frame->size();
					const auto finalPage = session->outgoing.front().finalPage;
					const auto handshakeKind = session->outgoing.front().handshakeKind;
					if (session->outgoing.front().reservation != nullptr)
					{
						session->outgoing.front().reservation->transition(CommandBudget::Stage::Frame);
					}
					session->outgoing.pop_front();
					session->outgoingOffset = 0;
					if (finalPage != 0)
					{
						session->transfers.finalPageWritten(finalPage);
						_releaseReservation(*session, finalPage);
					}
					if (handshakeKind != 0)
					{
						_handshakeWritten(*session, handshakeKind);
						if (!_sessions.contains(session->id))
						{
							return;
						}
					}
				}
				session->writeInFlight = false;
				_postWrite(session);
			}

			void _disconnect(ConnectionID id) noexcept
			{
				const auto iterator = _sessions.find(id);
				if (iterator == _sessions.end())
				{
					return;
				}
				auto session = std::move(iterator->second);
				_sessions.erase(iterator);
				session->socket.close();
				session->transfers.clear();
				session->reception.clear();
				_notifyDisconnection(id, session->handshake.error());
			}

			void _disconnectAll() noexcept
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

			void _enqueue(Command command)
			{
				{
					const std::scoped_lock lock(_commandMutex);
					if (command.batch != nullptr && !_running.load())
					{
						throw Exception("Unable to admit sends on a stopped network server.");
					}
					if (command.batch != nullptr)
					{
						const std::size_t bytes = std::max<std::size_t>(1, command.batch->message.size());
						command.reservation = _commandBudget.reserve(bytes, CommandBudget::Stage::Command);
					}
					_commands.push_back(std::move(command));
				}
				::PostQueuedCompletionStatus(_iocp, 0, WakeKey, nullptr);
			}

			[[nodiscard]] std::deque<Command> _takeCommands()
			{
				const std::scoped_lock lock(_commandMutex);
				std::deque<Command> result;
				result.swap(_commands);
				return result;
			}

			void _processCommands()
			{
				for (Command &command : _takeCommands())
				{
					_processCommand(std::move(command));
					if (_stopping)
					{
						return;
					}
				}
			}

			void _processCommand(Command command)
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

			void _sendBatch(ServerSendBatch &batch)
			{
				ServerSendAdmission<Session> admission(batch, _sessions, _commandBudget, _callbacks.snapshotConnections);
				admission.run(
					[this, &batch](const std::shared_ptr<Session> &session, ServerSendBatch::Recipient &recipient) {
						_queueMessage(session, batch.message, batch.priority, recipient);
					},
					[this](const std::shared_ptr<Session> &session) {
						if (!session->writeInFlight)
						{
							_postWrite(session);
						}
					},
					[this](ConnectionID id) {
						_disconnect(id);
					});
			}

			void _beginStop() noexcept
			{
				_stopping = true;
				_running = false;
				_listener.close();
				_disconnectAll();
				std::deque<Command> abandoned;
				{
					const std::scoped_lock lock(_commandMutex);
					abandoned.swap(_commands);
				}
			}

			void _handleCompletion(
				BOOL success,
				DWORD bytes,
				ULONG_PTR key,
				OVERLAPPED *overlapped)
			{
				if (overlapped == nullptr)
				{
					if (key == WakeKey)
					{
						_processCommands();
					}
					return;
				}

				std::unique_ptr<Operation> operation(
					reinterpret_cast<Operation *>(overlapped));
				if (_pendingOperations != 0)
				{
					--_pendingOperations;
				}

				switch (operation->kind)
				{
				case OperationKind::Accept:
					_completeAccept(std::move(operation), success != FALSE);
					break;
				case OperationKind::Read:
					_completeRead(operation->session, *operation, bytes, success != FALSE);
					break;
				case OperationKind::Write:
					_completeWrite(operation->session, bytes, success != FALSE);
					break;
				}
			}

			void _expireReassemblies()
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
				}
			}

			void _run() noexcept
			{
				while (!_stopping || _pendingOperations != 0)
				{
					DWORD bytes = 0;
					ULONG_PTR key = 0;
					OVERLAPPED *overlapped = nullptr;
					const BOOL success = ::GetQueuedCompletionStatus(
						_iocp,
						&bytes,
						&key,
						&overlapped,
						1000);
					try
					{
						if (overlapped != nullptr || success)
						{
							_handleCompletion(success, bytes, key, overlapped);
						}
						if (!_stopping)
						{
							_expireReassemblies();
						}
					} catch (...)
					{
						_beginStop();
					}
				}
			}

			void _openIocp()
			{
				_iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
				if (_iocp == nullptr)
				{
					throw Exception("Unable to create IOCP [" + std::to_string(::GetLastError()) + "].");
				}
			}

			void _closeIocp() noexcept
			{
				if (_iocp != nullptr)
				{
					::CloseHandle(_iocp);
					_iocp = nullptr;
				}
			}

		public:
			explicit IocpServerBackend(ServerCallbacks callbacks) :
				_callbacks(std::move(callbacks))
			{
			}

			~IocpServerBackend() override
			{
				stop();
			}

			[[nodiscard]] bool isRunning() const noexcept override
			{
				return _running.load();
			}

			[[nodiscard]] std::uint16_t port() const noexcept override
			{
				return _port.load();
			}

			void start(std::uint16_t port) override
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
				_pendingOperations = 0;
				_openIocp();
				try
				{
					_listener = Socket::listenTCP(port);
					_port = _listener.localPort();
					_associate(_listener);
					_loadAcceptEx();
					for (std::size_t index = 0; index < AcceptOperationCount; ++index)
					{
						_postAccept();
					}
				} catch (...)
				{
					_listener.close();
					_closeIocp();
					throw;
				}

				_running = true;
				_worker = std::jthread([this] {
					_run();
				});
			}

			void stop() override
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
					else if (_iocp != nullptr)
					{
						::PostQueuedCompletionStatus(_iocp, 0, WakeKey, nullptr);
					}
					_worker.join();
				}
				_listener.close();
				_disconnectAll();
				_closeIocp();
				_port = 0;
				_running = false;
				_stopping = true;
			}

			void setPageFailureLimits(PageFailureLimits limits) override
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

			void setHandshakeTimeout(std::chrono::milliseconds timeout) override
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

			void setIncomingTransferTimeout(std::chrono::milliseconds timeout) override
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

			void setAcknowledgmentTimeout(std::chrono::milliseconds timeout) override
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

			void setGlobalMaxQueuedBytes(std::size_t bytes) override
			{
				if (bytes == 0)
				{
					throw Exception("Global outgoing network memory limit must be positive.");
				}
				_maxGlobalQueuedBytes.store(bytes);
				_commandBudget.setMaximum(bytes);
			}

			void _configureIncomingLimits(std::size_t messageBytes, std::size_t incomingBytes)
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

			void setMaxMessageBytes(std::size_t bytes) override
			{
				if (bytes == 0)
				{
					throw Exception("Maximum logical message size must be positive.");
				}
				_configureIncomingLimits(bytes, _maxIncomingBytes.load());
			}

			void setMaxIncomingBytes(std::size_t bytes) override
			{
				if (bytes == 0)
				{
					throw Exception("Incoming reassembly budget must be positive.");
				}
				_configureIncomingLimits(_maxMessageBytes.load(), bytes);
			}

			void setMaxPageBytes(std::size_t bytes) override
			{
				if (bytes == 0 || bytes > MaximumSupportedPageBytes)
				{
					throw Exception("Network page size must be within the supported protocol limit.");
				}
				_maxPageBytes.store(bytes);
			}

			void setGlobalMaxIncomingBytes(std::size_t bytes) override
			{
				_incomingBudget->setMaximum(bytes);
			}

			void send(std::shared_ptr<ServerSendBatch> batch) override
			{
				if (!_running)
				{
					throw Exception("Unable to send through a stopped network server.");
				}
				if (_worker.get_id() == std::this_thread::get_id())
				{
					_sendBatch(*batch);
					return;
				}
				auto admitted = batch->admission.get_future();
				Command command{CommandKind::Send};
				command.batch = std::move(batch);
				_enqueue(std::move(command));
				try
				{
					admitted.get();
				} catch (const Exception &)
				{
					throw;
				} catch (...)
				{
					throw Exception("Network send admission failed.");
				}
			}
		};
	}

	std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks)
	{
		return std::make_unique<IocpServerBackend>(std::move(callbacks));
	}
}

#endif
