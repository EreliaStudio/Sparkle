#include "network/server.hpp"

#include "network/internal/server_backend.hpp"

#include <utility>

namespace spk
{
	class Server::Impl
	{
	private:
		std::unique_ptr<NetworkInternal::ServerBackend> _backend;

	public:
		explicit Impl(Server &owner) :
			_backend(NetworkInternal::createServerBackend({
				.onMessage = [&owner](ReceivedMessage message) {
					owner._publish(std::move(message));
				},
				.onConnection = [&owner](ConnectionID connection) {
					owner._notifyConnection(connection);
				},
				.onDisconnection = [&owner](ConnectionID connection) {
					owner._notifyDisconnection(connection);
				}}))
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

		void sendTo(ConnectionID connection, const Message &message)
		{
			_backend->sendTo(connection, message);
		}

		void sendToAll(const Message &message)
		{
			_backend->sendToAll(message);
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
