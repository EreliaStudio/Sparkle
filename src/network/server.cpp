#include "network/server.hpp"

#include "exception.hpp"
#include "network/internal/server_backend.hpp"

#include <utility>

namespace spk
{
	class Server::Impl
	{
	private:
		std::unique_ptr<NetworkInternal::ServerBackend> _backend;

		[[nodiscard]] static NetworkInternal::ServerCallbacks _callbacks(Server &owner)
		{
			NetworkInternal::ServerCallbacks result;
			result.onMessage = [&owner](ReceivedMessage message) {
				owner._publish(std::move(message));
			};
			result.onConnection = [&owner](ConnectionID connection) {
				owner._notifyConnection(connection);
			};
			result.onDisconnection = [&owner](ConnectionID connection) {
				owner._notifyDisconnection(connection);
			};
			return result;
		}

	public:
		explicit Impl(Server &owner) :
			_backend(NetworkInternal::createServerBackend(_callbacks(owner)))
		{
		}

		[[nodiscard]] bool isRunning() const noexcept
		{
			return _backend->isRunning();
		}

		[[nodiscard]] std::uint16_t port() const noexcept
		{
			return _backend->port();
		}

		void start(std::uint16_t port)
		{
			_backend->start(port);
		}

		void stop()
		{
			_backend->stop();
		}

		void sendTo(ConnectionID connection, const Message &message, Message::Priority priority)
		{
			_backend->sendTo(connection, message, priority);
		}

		void sendToAll(const Message &message, Message::Priority priority)
		{
			_backend->sendToAll(message, priority);
		}
	};

	Server::Server() :
		_impl(std::make_unique<Impl>(*this))
	{
	}

	Server::~Server()
	{
		stop();
	}

	Server::StatusContract Server::subscribeToConnectionStatusChange(StatusCallback callback)
	{
		return _statusProvider.subscribe(std::move(callback));
	}

	Connection Server::connection(ConnectionID id) const
	{
		const std::scoped_lock lock(_connectionsMutex);
		const auto it = _connections.find(id);
		if (it == _connections.end())
		{
			throw Exception("Unknown or disconnected network connection.");
		}
		return it->second;
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

	void Server::sendTo(const Connection &connection, const Message &message, Message::Priority priority)
	{
		{
			const std::scoped_lock lock(_connectionsMutex);
			const auto it = _connections.find(connection.id());
			if (it == _connections.end() || it->second._data != connection._data || !connection.isConnected())
			{
				throw Exception("Unable to send to an unknown or disconnected network connection.");
			}
		}
		_impl->sendTo(connection.id(), message, priority);
	}

	void Server::sendTo(const ConnectionSet &connections, const Message &message, Message::Priority priority)
	{
		const std::scoped_lock lock(_connectionsMutex);
		for (const Connection &connection : connections)
		{
			const auto it = _connections.find(connection.id());
			if (it == _connections.end() || it->second._data != connection._data || !connection.isConnected())
			{
				throw Exception("Unable to send to an unknown or disconnected network connection.");
			}
		}
		for (const Connection &connection : connections)
		{
			_impl->sendTo(connection.id(), message, priority);
		}
	}

	void Server::sendToAll(const Message &message, Message::Priority priority)
	{
		_impl->sendToAll(message, priority);
	}

	Server::MessageQueue &Server::messages() noexcept
	{
		return _messages;
	}

	Server::MessageDispatcher &Server::messageDispatcher() noexcept
	{
		return _messageDispatcher;
	}

	void Server::treatMessages()
	{
		_messageDispatcher.treatMessages(_messages);
	}

	void Server::_publish(ReceivedMessage message)
	{
		_messages.publish(std::move(message));
	}

	void Server::_notifyConnection(ConnectionID id)
	{
		Connection connection;
		connection._assignID(id);
		{
			const std::scoped_lock lock(_connectionsMutex);
			_connections.insert_or_assign(id, connection);
		}
		connection._setStatus(Connection::Status::Pending);
		_statusProvider.trigger(connection, Connection::Status::Pending, Connection::Error::None);
		{
			const std::scoped_lock lock(_connectionsMutex);
			if (!_connections.contains(id) || connection.status() != Connection::Status::Pending)
			{
				return;
			}
		}
		connection._setStatus(Connection::Status::Connected);
		_statusProvider.trigger(connection, Connection::Status::Connected, Connection::Error::None);
	}

	void Server::_notifyDisconnection(ConnectionID id)
	{
		Connection connection;
		{
			const std::scoped_lock lock(_connectionsMutex);
			const auto it = _connections.find(id);
			if (it == _connections.end())
			{
				return;
			}
			connection = it->second;
			_connections.erase(it);
		}
		connection._setStatus(Connection::Status::Disconnected);
		_statusProvider.trigger(connection, Connection::Status::Disconnected, Connection::Error::None);
	}
}
