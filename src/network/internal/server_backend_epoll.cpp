#include "network/internal/server_backend.hpp"

#if defined(__linux__) || defined(__ANDROID__)

#	include "exception.hpp"
#	include "network/internal/connection_id.hpp"
#	include <array>
#	include <cstddef>
#	include <cstdint>
#	include <memory>
#	include <string>
#	include <utility>
#	include <vector>
#	include <cerrno>
#	include <sys/epoll.h>
#	include <sys/eventfd.h>
#	include <sys/socket.h>
#	include <unistd.h>

namespace spk::NetworkInternal
{
	namespace
	{
		constexpr std::uint64_t WakeToken = 0;
		constexpr std::uint64_t ListenerToken = 1;
		constexpr std::size_t EventBatchSize = 128;
		constexpr std::size_t ReceptionBufferSize = 64u * 1024u;
		constexpr std::size_t WriteBudget = 256u * 1024u;

		class EpollServerBackend final : public ServerBackend
		{
		private:
			int _epoll = -1;
			int _wakeEvent = -1;

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

			void _unregisterSocket(const Socket &socket) noexcept override
			{
				if (_epoll >= 0 && socket.valid())
				{
					::epoll_ctl(_epoll, EPOLL_CTL_DEL, static_cast<int>(socket.nativeHandle()), nullptr);
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
				auto session = _makeSession(std::move(socket));
				const ConnectionID id = session->id;
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
				try
				{
					while (_sessions.contains(session->id))
					{
						const auto received = session->socket.receiveSome(buffer);
						if (!received.has_value())
						{
							return;
						}
						if (received->disconnected)
						{
							_disconnect(session->id);
							return;
						}
						_appendReceived(*session, buffer.data(), received->size);
					}
				} catch (...)
				{
					_disconnect(session->id);
				}
			}

			void _writeSession(const std::shared_ptr<Session> &session) override
			{
				std::size_t budget = WriteBudget;
				while (budget != 0 && _prepareWrite(*session))
				{
					const auto sent = session->socket.sendSome(std::span<const std::byte>(*session->outgoing.front().bytes).subspan(session->outgoingOffset));
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
					budget = *sent >= budget ? 0 : budget - *sent;
					_completeFrame(*session, *sent);
					if (!_sessions.contains(session->id))
					{
						return;
					}
				}
				_modify(*session, !session->outgoing.empty() || session->transfers.pendingTransfers() != 0);
			}

			void _wake() noexcept override
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

			void _handleEvent(const epoll_event &event)
			{
				if (event.data.u64 == WakeToken)
				{
					_drainWake();
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
						_writeSession(session);
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

			void _run() noexcept override
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

			void _openEvents() override
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

			void _closeEvents() noexcept override
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

			void _requestWrite(Session &session) override
			{
				_modify(session, true);
			}

			void _listen() override
			{
				_listener.setNonBlocking(true);
				_register(static_cast<int>(_listener.nativeHandle()), ListenerToken, EPOLLIN | EPOLLERR);
			}

		public:
			explicit EpollServerBackend(ServerCallbacks callbacks) :
				ServerBackend(std::move(callbacks))
			{
			}

			~EpollServerBackend() override
			{
				stop();
			}
		};
	}

	std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks)
	{
		return std::make_unique<EpollServerBackend>(std::move(callbacks));
	}
}

#endif
