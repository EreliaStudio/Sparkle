#include "network/server.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/socket.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	class Server::Impl
	{
	private:
		struct Session
		{
			ConnectionID id;
			NetworkInternal::Socket socket;
			std::vector<std::byte> receivedBytes;
			std::mutex sendMutex;

			Session(ConnectionID id, NetworkInternal::Socket socket) :
				id(id),
				socket(std::move(socket))
			{
			}
		};

		static constexpr auto PollInterval = std::chrono::milliseconds(50);
		static constexpr std::size_t ReceptionBufferSize = 64u * 1024u;

		Server &_owner;
		NetworkInternal::Socket _listener;
		std::jthread _worker;
		std::atomic_bool _running = false;
		std::atomic_uint16_t _port = 0;
		ConnectionID _nextConnectionID = 1;
		std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;
		std::mutex _sessionsMutex;

		void _run() noexcept
		{
			try
			{
				while (_running)
				{
					const auto sessions = _sessionsSnapshot();
					std::vector<const NetworkInternal::Socket *> sockets;
					sockets.reserve(sessions.size() + 1);
					sockets.push_back(&_listener);
					for (const auto &session : sessions)
					{
						sockets.push_back(&session->socket);
					}

					const auto ready = NetworkInternal::waitReadable(sockets, PollInterval);
					for (const std::size_t index : ready)
					{
						if (!_running)
						{
							break;
						}
						if (index == 0)
						{
							_accept();
						}
						else if (index <= sessions.size())
						{
							_receive(sessions[index - 1]);
						}
					}
				}
			}
			catch (...)
			{
				_running = false;
			}
			_disconnectAll();
		}

		[[nodiscard]] std::vector<std::shared_ptr<Session>> _sessionsSnapshot()
		{
			const std::scoped_lock lock(_sessionsMutex);
			std::vector<std::shared_ptr<Session>> result;
			result.reserve(_sessions.size());
			for (const auto &entry : _sessions)
			{
				result.push_back(entry.second);
			}
			return result;
		}

		void _accept()
		{
			auto socket = _listener.accept();
			const ConnectionID id = _nextConnectionID++;
			auto session = std::make_shared<Session>(id, std::move(socket));

			{
				const std::scoped_lock lock(_sessionsMutex);
				_sessions.emplace(id, session);
			}

			try
			{
				_owner._notifyConnection(id);
			}
			catch (...)
			{
			}
		}

		void _receive(const std::shared_ptr<Session> &session)
		{
			std::array<std::byte, ReceptionBufferSize> buffer{};
			try
			{
				const auto result = session->socket.receive(buffer);
				if (result.disconnected)
				{
					_disconnect(session->id);
					return;
				}

				session->receivedBytes.insert(
					session->receivedBytes.end(),
					buffer.begin(),
					buffer.begin() + static_cast<std::ptrdiff_t>(result.size));
				_extractMessages(*session);
			}
			catch (...)
			{
				_disconnect(session->id);
			}
		}

		void _extractMessages(Session &session)
		{
			while (session.receivedBytes.size() >= NetworkInternal::FrameHeaderSize)
			{
				const auto headerBytes = std::span<const std::byte, NetworkInternal::FrameHeaderSize>(
					session.receivedBytes.data(),
					NetworkInternal::FrameHeaderSize);
				const auto header = NetworkInternal::decode(headerBytes);
				const std::size_t frameSize = NetworkInternal::FrameHeaderSize + header.payloadSize;
				if (session.receivedBytes.size() < frameSize)
				{
					return;
				}

				Message::Storage payload(header.payloadSize);
				std::copy_n(
					session.receivedBytes.begin() + NetworkInternal::FrameHeaderSize,
					header.payloadSize,
					payload.begin());
				_owner._publish(ReceivedMessage{session.id, Message(header.type, std::move(payload))});
				session.receivedBytes.erase(
					session.receivedBytes.begin(),
					session.receivedBytes.begin() + static_cast<std::ptrdiff_t>(frameSize));
			}
		}

		void _disconnect(ConnectionID id) noexcept
		{
			std::shared_ptr<Session> session;
			{
				const std::scoped_lock lock(_sessionsMutex);
				const auto iterator = _sessions.find(id);
				if (iterator == _sessions.end())
				{
					return;
				}
				session = std::move(iterator->second);
				_sessions.erase(iterator);
			}

			session->socket.close();
			try
			{
				_owner._notifyDisconnection(id);
			}
			catch (...)
			{
			}
		}

		void _disconnectAll() noexcept
		{
			auto sessions = _sessionsSnapshot();
			{
				const std::scoped_lock lock(_sessionsMutex);
				_sessions.clear();
			}

			for (const auto &session : sessions)
			{
				session->socket.close();
				try
				{
					_owner._notifyDisconnection(session->id);
				}
				catch (...)
				{
				}
			}
		}

		[[nodiscard]] std::shared_ptr<Session> _session(ConnectionID id)
		{
			const std::scoped_lock lock(_sessionsMutex);
			const auto iterator = _sessions.find(id);
			return iterator == _sessions.end() ? nullptr : iterator->second;
		}

		void _ensureRunning() const
		{
			if (!_running)
			{
				throw Exception("Unable to send through a stopped network server.");
			}
		}

		void _send(const std::shared_ptr<Session> &session, const Message &message)
		{
			const auto frame = NetworkInternal::encode(message);
			const std::scoped_lock lock(session->sendMutex);
			session->socket.sendAll(*frame);
		}

	public:
		explicit Impl(Server &owner) :
			_owner(owner)
		{
		}

		~Impl()
		{
			try
			{
				stop();
			}
			catch (...)
			{
			}
		}

		[[nodiscard]] bool isRunning() const noexcept
		{
			return _running.load();
		}

		[[nodiscard]] std::uint16_t port() const noexcept
		{
			return _port.load();
		}

		void start(std::uint16_t requestedPort)
		{
			stop();

			try
			{
				_listener = NetworkInternal::Socket::listenTCP(requestedPort);
				_port = _listener.localPort();
			}
			catch (...)
			{
				throw Exception("Unable to start the network server.", std::current_exception());
			}

			_running = true;
			_worker = std::jthread([this] {
				_run();
			});
		}

		void stop()
		{
			_running = false;
			if (_worker.joinable() && _worker.get_id() != std::this_thread::get_id())
			{
				_worker.join();
			}
			_listener.close();
			_port = 0;
		}

		void sendTo(ConnectionID connection, const Message &message)
		{
			_ensureRunning();
			const auto session = _session(connection);
			if (session == nullptr)
			{
				return;
			}

			try
			{
				_send(session, message);
			}
			catch (...)
			{
				_disconnect(connection);
			}
		}

		void sendToAll(const Message &message)
		{
			_ensureRunning();
			for (const auto &session : _sessionsSnapshot())
			{
				try
				{
					_send(session, message);
				}
				catch (...)
				{
					_disconnect(session->id);
				}
			}
		}
	};

	Server::Server() :
		_impl(std::make_unique<Impl>(*this))
	{
	}

	Server::~Server() = default;

	Server::ConnectionContract Server::subscribeToConnection(ConnectionCallback callback)
	{
		return _connectionProvider.subscribe(std::move(callback));
	}

	Server::DisconnectionContract Server::subscribeToDisconnection(DisconnectionCallback callback)
	{
		return _disconnectionProvider.subscribe(std::move(callback));
	}

	bool Server::isRunning() const noexcept
	{
		return _impl->isRunning();
	}

	std::uint16_t Server::port() const noexcept
	{
		return _impl->port();
	}

	void Server::start(std::uint16_t port)
	{
		_impl->start(port);
	}

	void Server::stop()
	{
		_impl->stop();
	}

	void Server::sendTo(ConnectionID connection, const Message &message)
	{
		_impl->sendTo(connection, message);
	}

	void Server::sendTo(std::span<const ConnectionID> connections, const Message &message)
	{
		for (const ConnectionID connection : connections)
		{
			sendTo(connection, message);
		}
	}

	void Server::sendToAll(const Message &message)
	{
		_impl->sendToAll(message);
	}

	Server::MessageQueue &Server::messages() noexcept
	{
		return _messages;
	}

	void Server::_publish(ReceivedMessage message)
	{
		_messages.publish(std::move(message));
	}

	void Server::_notifyConnection(ConnectionID connection)
	{
		_connectionProvider.trigger(connection);
	}

	void Server::_notifyDisconnection(ConnectionID connection)
	{
		_disconnectionProvider.trigger(connection);
	}
}
