#include "network/internal/server_backend.hpp"

#if defined(__linux__) || defined(__ANDROID__)

#	include "exception.hpp"
#	include "network/internal/command_budget.hpp"
#	include "network/internal/connection_id.hpp"
#	include "network/internal/frame.hpp"
#	include "network/internal/handshake.hpp"
#	include "network/internal/server_handshake.hpp"
#	include "network/internal/page.hpp"
#	include "network/internal/transmission_frame.hpp"
#	include "network/internal/transfer_reception.hpp"
#	include "network/internal/transfer_sender.hpp"
#	include "network/internal/socket.hpp"

#	include <algorithm>
#	include <array>
#	include <atomic>
#	include <cerrno>
#	include <chrono>
#	include <cstddef>
#	include <cstdint>
#	include <deque>
#	include <exception>
#	include <future>
#	include <memory>
#	include <mutex>
#	include <optional>
#	include <string>
#	include <sys/epoll.h>
#	include <sys/eventfd.h>
#	include <sys/socket.h>
#	include <thread>
#	include <unordered_map>
#	include <unistd.h>
#	include <utility>
#	include <vector>

namespace spk::NetworkInternal
{
	namespace
	{
		using Frame = std::shared_ptr<std::vector<std::byte>>;

		constexpr std::uint64_t WakeToken = 0;
		constexpr std::uint64_t ListenerToken = 1;
		constexpr std::size_t EventBatchSize = 128;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t MaximumQueuedBytes = 64u * 1024u * 1024u;
		constexpr std::size_t WriteBudget = 256u * 1024u;

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

			Session(ConnectionID id, Socket socket, PageFailureLimits limits, std::chrono::milliseconds timeout, std::shared_ptr<ReassemblyBudget> sharedBudget, std::uint32_t maximumPageBytes) :
				id(id),
				socket(std::move(socket)),
				reception(std::move(sharedBudget)),
				handshake(timeout, maximumPageBytes)
			{
				reception.configureFailureLimits(limits);
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

		class EpollServerBackend final : public ServerBackend
		{
		private:
			ServerCallbacks _callbacks;
			Socket _listener;
			std::jthread _worker;
			std::atomic_bool _running = false;
			std::atomic_uint16_t _port = 0;
			int _epoll = -1;
			int _wakeEvent = -1;
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
			bool _stopping = false;

			[[nodiscard]] static std::uint64_t _token(ConnectionID id)
			{
				return id + ConnectionTokenOffset;
			}

			[[nodiscard]] static ConnectionID _connection(std::uint64_t token)
			{
				return token - ConnectionTokenOffset;
			}

			void _register(int handle, std::uint64_t token, std::uint32_t events)
			{
				epoll_event event{};
				event.events = events;
				event.data.u64 = token;
				if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, handle, &event) != 0)
				{
					throw Exception("Unable to register epoll socket [" + std::to_string(errno) + "].");
				}
			}

			void _modify(const Session &session, bool writable)
			{
				epoll_event event{};
				event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
				if (writable)
				{
					event.events |= EPOLLOUT;
				}
				event.data.u64 = _token(session.id);
				::epoll_ctl(_epoll, EPOLL_CTL_MOD, static_cast<int>(session.socket.nativeHandle()), &event);
			}

			void _unregister(const Socket &socket) noexcept
			{
				if (_epoll >= 0 && socket.valid())
				{
					::epoll_ctl(_epoll, EPOLL_CTL_DEL, static_cast<int>(socket.nativeHandle()), nullptr);
				}
			}

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

			void _acceptConnections()
			{
				while (!_stopping)
				{
					const int handle = ::accept(
						static_cast<int>(_listener.nativeHandle()), nullptr, nullptr);
					if (handle >= 0)
					{
						_addSession(Socket::adopt(static_cast<Socket::NativeHandle>(handle)));
						continue;
					}
					if (errno == EINTR)
					{
						continue;
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						return;
					}
					throw Exception("Unable to accept TCP connection [" + std::to_string(errno) + "].");
				}
			}

			void _addSession(Socket socket)
			{
				socket.setNonBlocking(true);
				const ConnectionID id = allocateConnectionID();
				auto session = std::make_shared<Session>(id, std::move(socket), _failureLimits, _handshakeTimeout, _incomingBudget, static_cast<std::uint32_t>(_maxPageBytes.load()));
				session->reception.setInactivityTimeout(_incomingTransferTimeout);
				session->reception.setLimits(_maxMessageBytes.load(), _maxIncomingBytes.load());
				session->transfers.setMaxMessageBytes(_maxMessageBytes.load());
				session->transfers.setAcknowledgmentTimeout(_acknowledgmentTimeout);
				_register(
					static_cast<int>(session->socket.nativeHandle()),
					_token(id),
					EPOLLIN | EPOLLRDHUP | EPOLLERR);
				_sessions.emplace(id, session);
				_notifyConnection(id);
			}

			void _readSession(const std::shared_ptr<Session> &session)
			{
				std::array<std::byte, ReceptionBufferSize> buffer{};
				while (true)
				{
					const auto result = ::recv(
						static_cast<int>(session->socket.nativeHandle()),
						buffer.data(),
						buffer.size(),
						0);
					if (result > 0)
					{
						try
						{
							_appendReceived(*session, buffer.data(), static_cast<std::size_t>(result));
						} catch (...)
						{
							_disconnect(session->id);
							return;
						}
						if (!_sessions.contains(session->id))
						{
							return;
						}
						continue;
					}
					if (result == 0)
					{
						_disconnect(session->id);
						return;
					}
					if (errno == EINTR)
					{
						continue;
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						return;
					}
					_disconnect(session->id);
					return;
				}
			}

			void _appendReceived(Session &session, const std::byte *data, std::size_t size)
			{
				session.receivedBytes.insert(session.receivedBytes.end(), data, data + size);
				_extractMessages(session);
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
				session.outgoing.push_back(TransmissionFrame{std::move(frame), 0, static_cast<std::uint16_t>(kind)});
				_modify(session, true);
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
				session.outgoing.push_back(TransmissionFrame{std::move(frame), 0});
				_modify(session, true);
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

			void _flushSession(const std::shared_ptr<Session> &session)
			{
				std::size_t budget = WriteBudget;
				while (budget != 0)
				{
					if (session->outgoing.empty())
					{
						auto page = session->transfers.nextPage((std::min)(_maxPageBytes.load(), static_cast<std::size_t>(session->handshake.maximumPageBytes())));
						if (!page.has_value())
						{
							break;
						}
						Frame frame = encodePage(*page);
						session->queuedBytes += frame->size();
						session->outgoing.push_back(TransmissionFrame{std::move(frame), page->offset + page->length == page->totalSize ? page->transferID : 0});
					}
					const Frame &frame = session->outgoing.front().bytes;
					const auto sent = _send(*session, *frame);
					if (!sent.has_value())
					{
						_modify(*session, true);
						return;
					}
					if (*sent == 0)
					{
						_disconnect(session->id);
						return;
					}
					session->outgoingOffset += *sent;
					budget = *sent >= budget ? 0 : budget - *sent;
					if (session->outgoingOffset == frame->size())
					{
						session->queuedBytes -= frame->size();
						const auto finalPage = session->outgoing.front().finalPage;
						const auto handshakeKind = session->outgoing.front().handshakeKind;
						session->outgoing.pop_front();
						session->outgoingOffset = 0;
						if (finalPage != 0)
						{
							session->transfers.finalPageWritten(finalPage);
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
				}
				_modify(*session, !session->outgoing.empty() || session->transfers.pendingTransfers() != 0);
			}

			[[nodiscard]] std::optional<std::size_t> _send(
				const Session &session,
				const std::vector<std::byte> &frame)
			{
				while (true)
				{
					const auto *data = frame.data() + session.outgoingOffset;
					const std::size_t size = frame.size() - session.outgoingOffset;
					const auto sent = ::send(
						static_cast<int>(session.socket.nativeHandle()),
						data,
						size,
						MSG_NOSIGNAL);
					if (sent >= 0)
					{
						return static_cast<std::size_t>(sent);
					}
					if (errno == EINTR)
					{
						continue;
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						return std::nullopt;
					}
					throw Exception("Unable to send TCP data [" + std::to_string(errno) + "].");
				}
			}

			void _queueMessage(const std::shared_ptr<Session> &session, const Message &message, Message::Priority priority, spk::Transfer transfer)
			{
				if (!session->handshake.ready())
				{
					throw Exception("Cannot send to a pending network connection.");
				}
				(void)session->transfers.enqueue(message.withConnectionID(session->id), priority, std::move(transfer));
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
				_unregister(session->socket);
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
					if (command.batch != nullptr)
					{
						const std::size_t count = std::max<std::size_t>(1, command.batch->recipients.size());
						const std::size_t bytes = std::max<std::size_t>(1, command.batch->message.size());
						const std::size_t limit = _maxGlobalQueuedBytes.load();
						if (count == 0 || bytes > limit / count)
						{
							throw Exception("Network outgoing command exceeds the global queue limit.");
						}
						command.reservation = _commandBudget.reserve(bytes * count);
					}
					_commands.push_back(std::move(command));
				}
				_wake();
			}

			void _wake() noexcept
			{
				if (_wakeEvent < 0)
				{
					return;
				}
				const std::uint64_t value = 1;
				const auto result = ::write(_wakeEvent, &value, sizeof(value));
				(void)result;
			}

			void _drainWake() noexcept
			{
				std::uint64_t value = 0;
				while (::read(_wakeEvent, &value, sizeof(value)) < 0 && errno == EINTR)
				{
				}
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
				_drainWake();
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
				if (batch.allRecipients)
				{
					const auto connections = _callbacks.snapshotConnections();
					if (connections.empty() && batch.policy != Connection::OverflowPolicy::SkipRecipient)
					{
						throw Exception("Cannot broadcast without connected recipients.");
					}
					batch.recipients.reserve(connections.size());
					for (const Connection &connection : connections)
					{
						const auto it = _sessions.find(connection.id());
						if (!connection.isConnected() || it == _sessions.end() || !it->second->handshake.ready())
						{
							if (batch.policy == Connection::OverflowPolicy::SkipRecipient)
							{
								continue;
							}
							throw Exception("Cannot broadcast while a recipient is pending or disconnected.");
						}
						batch.recipients.push_back({connection, spk::Transfer{}});
					}
				}
				struct Candidate
				{
					ServerSendBatch::Recipient *recipient;
					std::shared_ptr<Session> session;
				};
				std::vector<Candidate> candidates;
				std::vector<ConnectionID> overflowed;
				std::vector<std::pair<ServerSendBatch::Recipient *, spk::Transfer::Reason>> declined;
				std::size_t used = 0;
				for (const auto &[id, session] : _sessions)
				{
					(void)id;
					used += session->queuedBytes + session->transfers.pendingBytes();
				}
				const std::size_t limit = _maxGlobalQueuedBytes.load();
				const std::size_t size = std::max<std::size_t>(1, batch.message.size());
				bool rejected = false;
				for (auto &recipient : batch.recipients)
				{
					const auto it = _sessions.find(recipient.connection.id());
					if (it == _sessions.end() || !it->second->handshake.ready())
					{
						throw Exception("Cannot send to an invalid, pending or disconnected recipient.");
					}
					const auto &session = it->second;
					const std::size_t pending = session->queuedBytes + session->transfers.pendingBytes();
					const std::size_t budget = recipient.connection.maxQueuedBytes();
					if (pending > budget || size > budget - pending || used > limit || size > limit - used)
					{
						declined.push_back({&recipient, spk::Transfer::Reason::QueueOverflow});
						rejected = true;
						if (batch.policy == Connection::OverflowPolicy::DisconnectRecipient)
						{
							overflowed.push_back(session->id);
						}
						continue;
					}
					used += size;
					candidates.push_back({&recipient, session});
				}
				if (rejected && batch.policy == Connection::OverflowPolicy::Reject)
				{
					throw Exception("Insufficient capacity for atomic network broadcast admission.");
				}
				try
				{
					for (auto &candidate : candidates)
					{
						candidate.session->transfers.setMaxPendingBytes(candidate.recipient->connection.maxQueuedBytes());
						_queueMessage(candidate.session, batch.message, batch.priority, candidate.recipient->transfer);
						candidate.recipient->admitted = true;
					}
				} catch (...)
				{
					for (auto &candidate : candidates)
					{
						if (candidate.recipient->admitted)
						{
							(void)candidate.session->transfers.fail(candidate.recipient->transfer.id(), spk::Transfer::Status::Rejected, spk::Transfer::Reason::QueueOverflow);
						}
						else
						{
							(void)TransferCompletion::complete(candidate.recipient->transfer, spk::Transfer::Status::Rejected, spk::Transfer::Reason::QueueOverflow);
						}
					}
					for (const auto &[recipient, reason] : declined)
					{
						(void)TransferCompletion::complete(recipient->transfer, spk::Transfer::Status::Rejected, reason);
					}
					return;
				}
				for (const auto &candidate : candidates)
				{
					try
					{
						_flushSession(candidate.session);
					} catch (...)
					{
						_disconnect(candidate.session->id);
					}
				}
				for (const ConnectionID id : overflowed)
				{
					_disconnect(id);
				}
				for (const auto &[recipient, reason] : declined)
				{
					(void)TransferCompletion::complete(recipient->transfer, spk::Transfer::Status::Rejected, reason);
				}
			}

			void _beginStop() noexcept
			{
				_stopping = true;
				_running = false;
				_unregister(_listener);
				_listener.close();
				_disconnectAll();
			}

			void _handleEvent(const epoll_event &event)
			{
				if (event.data.u64 == WakeToken)
				{
					_processCommands();
					return;
				}
				if (event.data.u64 == ListenerToken)
				{
					_acceptConnections();
					return;
				}
				_handleSessionEvent(_connection(event.data.u64), event.events);
			}

			void _handleSessionEvent(ConnectionID id, std::uint32_t events)
			{
				const auto iterator = _sessions.find(id);
				if (iterator == _sessions.end())
				{
					return;
				}
				const auto session = iterator->second;
				if ((events & EPOLLIN) != 0)
				{
					_readSession(session);
				}
				if (_sessions.contains(id) && (events & EPOLLOUT) != 0)
				{
					try
					{
						_flushSession(session);
					} catch (...)
					{
						_disconnect(id);
					}
				}
				if (_sessions.contains(id) &&
					(events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0)
				{
					_disconnect(id);
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
				for (const ConnectionID id : ids)
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
						_queueResponse(*session, response);
					}
					session->transfers.expire();
				}
			}

			void _run() noexcept
			{
				std::array<epoll_event, EventBatchSize> events{};
				try
				{
					while (!_stopping)
					{
						const int count = ::epoll_wait(_epoll, events.data(), events.size(), 1000);
						if (count < 0 && errno == EINTR)
						{
							continue;
						}
						if (count < 0)
						{
							throw Exception("Unable to wait for epoll events [" + std::to_string(errno) + "].");
						}
						for (int index = 0; index < count && !_stopping; ++index)
						{
							_handleEvent(events[static_cast<std::size_t>(index)]);
						}
						_expireReassemblies();
					}
				} catch (...)
				{
					_beginStop();
				}
			}

			void _openKernelEvents()
			{
				_epoll = ::epoll_create1(EPOLL_CLOEXEC);
				if (_epoll < 0)
				{
					throw Exception("Unable to create epoll instance [" + std::to_string(errno) + "].");
				}
				_wakeEvent = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
				if (_wakeEvent < 0)
				{
					throw Exception("Unable to create network eventfd [" + std::to_string(errno) + "].");
				}
				_register(_wakeEvent, WakeToken, EPOLLIN);
			}

			void _closeKernelEvents() noexcept
			{
				if (_wakeEvent >= 0)
				{
					::close(_wakeEvent);
					_wakeEvent = -1;
				}
				if (_epoll >= 0)
				{
					::close(_epoll);
					_epoll = -1;
				}
			}

		public:
			explicit EpollServerBackend(ServerCallbacks callbacks) :
				_callbacks(std::move(callbacks))
			{
			}

			~EpollServerBackend() override
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
				_openKernelEvents();
				try
				{
					_listener = Socket::listenTCP(port);
					_listener.setNonBlocking(true);
					_port = _listener.localPort();
					_register(
						static_cast<int>(_listener.nativeHandle()),
						ListenerToken,
						EPOLLIN | EPOLLERR);
				} catch (...)
				{
					_listener.close();
					_closeKernelEvents();
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
					else
					{
						_wake();
					}
					_worker.join();
				}
				_listener.close();
				_disconnectAll();
				_closeKernelEvents();
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
		return std::make_unique<EpollServerBackend>(std::move(callbacks));
	}
}

#endif
