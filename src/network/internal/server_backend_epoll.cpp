#include "network/internal/server_backend.hpp"

#if defined(__linux__) || defined(__ANDROID__)

#	include "exception.hpp"
#	include "network/internal/frame.hpp"
#	include "network/internal/socket.hpp"

#	include <algorithm>
#	include <array>
#	include <atomic>
#	include <cerrno>
#	include <cstddef>
#	include <cstdint>
#	include <deque>
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
		constexpr std::uint64_t ConnectionTokenOffset = 2;
		constexpr std::size_t EventBatchSize = 128;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t MaximumQueuedBytes = 64u * 1024u * 1024u;
		constexpr std::size_t WriteBudget = 256u * 1024u;

		struct Session
		{
			ConnectionID id;
			Socket socket;
			std::vector<std::byte> receivedBytes;
			std::deque<Frame> outgoing;
			std::size_t outgoingOffset = 0;
			std::size_t queuedBytes = 0;

			Session(ConnectionID id, Socket socket) :
				id(id),
				socket(std::move(socket))
			{
			}
		};

		enum class CommandKind
		{
			Send,
			Broadcast,
			Stop
		};

		struct Command
		{
			CommandKind kind;
			ConnectionID connection = InvalidConnectionID;
			Frame frame;
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
			ConnectionID _nextConnectionID = 1;
			std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;
			std::mutex _commandMutex;
			std::deque<Command> _commands;
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

			void _notifyDisconnection(ConnectionID id) noexcept
			{
				try
				{
					_callbacks.onDisconnection(id);
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
				const ConnectionID id = _nextConnectionID++;
				auto session = std::make_shared<Session>(id, std::move(socket));
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
						_appendReceived(*session, buffer.data(), static_cast<std::size_t>(result));
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
				if (remaining < frameSize)
				{
					return false;
				}

				Message::Storage payload(header.payloadSize);
				std::copy_n(data + FrameHeaderSize, header.payloadSize, payload.begin());
				consumed += frameSize;
				Message message(header.type, std::move(payload));
				message.setRequestID(header.requestID);
				_publish(ReceivedMessage{session.id, std::move(message)});
				return true;
			}

			void _flushSession(const std::shared_ptr<Session> &session)
			{
				std::size_t budget = WriteBudget;
				while (!session->outgoing.empty() && budget != 0)
				{
					const Frame &frame = session->outgoing.front();
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
					session->queuedBytes -= *sent;
					budget = *sent >= budget ? 0 : budget - *sent;
					if (session->outgoingOffset == frame->size())
					{
						session->outgoing.pop_front();
						session->outgoingOffset = 0;
					}
				}
				_modify(*session, !session->outgoing.empty());
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

			void _queueFrame(const std::shared_ptr<Session> &session, Frame frame)
			{
				if (frame->size() > MaximumQueuedBytes - session->queuedBytes)
				{
					_disconnect(session->id);
					return;
				}
				const bool wasEmpty = session->outgoing.empty();
				session->queuedBytes += frame->size();
				session->outgoing.push_back(std::move(frame));
				if (wasEmpty)
				{
					try
					{
						_flushSession(session);
					} catch (...)
					{
						_disconnect(session->id);
					}
				}
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
				_notifyDisconnection(id);
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
				if (command.kind == CommandKind::Broadcast)
				{
					_broadcast(std::move(command.frame));
					return;
				}

				const auto iterator = _sessions.find(command.connection);
				if (iterator != _sessions.end())
				{
					_queueFrame(iterator->second, std::move(command.frame));
				}
			}

			void _broadcast(Frame frame)
			{
				std::vector<std::shared_ptr<Session>> sessions;
				sessions.reserve(_sessions.size());
				for (const auto &entry : _sessions)
				{
					sessions.push_back(entry.second);
				}
				for (const auto &session : sessions)
				{
					if (_sessions.contains(session->id))
					{
						_queueFrame(session, frame);
					}
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

			void _run() noexcept
			{
				std::array<epoll_event, EventBatchSize> events{};
				try
				{
					while (!_stopping)
					{
						const int count = ::epoll_wait(_epoll, events.data(), events.size(), -1);
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

			void sendTo(ConnectionID connection, const Message &message) override
			{
				if (!_running)
				{
					throw Exception("Unable to send through a stopped network server.");
				}
				_enqueue(Command{CommandKind::Send, connection, encode(message)});
			}

			void sendToAll(const Message &message) override
			{
				if (!_running)
				{
					throw Exception("Unable to send through a stopped network server.");
				}
				_enqueue(Command{CommandKind::Broadcast, InvalidConnectionID, encode(message)});
			}
		};
	}

	std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks)
	{
		return std::make_unique<EpollServerBackend>(std::move(callbacks));
	}
}

#endif
