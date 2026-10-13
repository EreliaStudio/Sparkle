#include "network/server.hpp"

#include "exception.hpp"
#include "network/internal/outgoing_message_validator.hpp"
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
			result.onReady = [&owner](ConnectionID connection) {
				owner._notifyReady(connection);
			};
			result.onDisconnection = [&owner](ConnectionID connection, Connection::Error error) {
				owner._notifyDisconnection(connection, error);
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

		void setPageFailureLimits(PageFailureLimits limits)
		{
			_backend->setPageFailureLimits(limits);
		}

		void setHandshakeTimeout(std::chrono::milliseconds timeout)
		{
			_backend->setHandshakeTimeout(timeout);
		}

		void setIncomingTransferTimeout(std::chrono::milliseconds timeout)
		{
			_backend->setIncomingTransferTimeout(timeout);
		}

		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
		{
			_backend->setAcknowledgmentTimeout(timeout);
		}

		void setGlobalMaxQueuedBytes(std::size_t bytes)
		{
			_backend->setGlobalMaxQueuedBytes(bytes);
		}

		void setMaxMessageBytes(std::size_t bytes)
		{
			_backend->setMaxMessageBytes(bytes);
		}
		void setMaxIncomingBytes(std::size_t bytes)
		{
			_backend->setMaxIncomingBytes(bytes);
		}
		void setGlobalMaxIncomingBytes(std::size_t bytes)
		{
			_backend->setGlobalMaxIncomingBytes(bytes);
		}
		void send(std::shared_ptr<NetworkInternal::ServerSendBatch> batch)
		{
			_backend->send(std::move(batch));
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

	void Server::setPageFailureLimits(PageFailureLimits limits)
	{
		_impl->setPageFailureLimits(limits);
	}

	void Server::setHandshakeTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setHandshakeTimeout(timeout);
	}

	void Server::setIncomingTransferTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setIncomingTransferTimeout(timeout);
	}

	void Server::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setAcknowledgmentTimeout(timeout);
	}

	void Server::setGlobalMaxQueuedBytes(std::size_t bytes)
	{
		_impl->setGlobalMaxQueuedBytes(bytes);
	}

	void Server::setMaxMessageBytes(std::size_t bytes)
	{
		_impl->setMaxMessageBytes(bytes);
		_maxMessageBytes.store(bytes);
	}
	void Server::setMaxIncomingBytes(std::size_t bytes)
	{
		_impl->setMaxIncomingBytes(bytes);
	}
	void Server::setGlobalMaxIncomingBytes(std::size_t bytes)
	{
		_impl->setGlobalMaxIncomingBytes(bytes);
	}
	Transfer Server::sendTo(const Connection &connection, const Message &message, Message::Priority priority)
	{
		const ConnectionSet recipients{connection};
		return sendTo(recipients, message, priority).at(connection.id());
	}

	Server::TransferResults Server::sendTo(const ConnectionSet &connections, const Message &message, Message::Priority priority, Connection::OverflowPolicy policy)
	{
		if (connections.empty())
		{
			throw Exception("Cannot send to an empty connection set.");
		}
		NetworkInternal::OutgoingMessageValidator::validate(message, priority, _maxMessageBytes.load());
		auto batch = std::make_shared<NetworkInternal::ServerSendBatch>(message, priority, policy);
		TransferResults results;
		batch->recipients.reserve(connections.size());
		results.reserve(connections.size());
		{
			const std::scoped_lock lock(_connectionsMutex);
			for (const Connection &connection : connections)
			{
				const auto it = _connections.find(connection.id());
				if (it == _connections.end() || it->second._data != connection._data || !connection.isConnected())
				{
					throw Exception("Unable to send to an unknown or disconnected network connection.");
				}
				if (policy == Connection::OverflowPolicy::Reject)
				{
					NetworkInternal::OutgoingMessageValidator::validateCapacity(message, connection.maxQueuedBytes());
				}
				batch->recipients.push_back({connection, Transfer{}});
				results.emplace(connection.id(), batch->recipients.back().transfer);
			}
		}
		_impl->send(std::move(batch));
		return results;
	}

	Server::TransferResults Server::sendToAll(const Message &message, Message::Priority priority, Connection::OverflowPolicy policy)
	{
		NetworkInternal::OutgoingMessageValidator::validate(message, priority, _maxMessageBytes.load());
		auto batch = std::make_shared<NetworkInternal::ServerSendBatch>(message, priority, policy);
		TransferResults results;
		{
			const std::scoped_lock lock(_connectionsMutex);
			if (_connections.empty())
			{
				throw Exception("Cannot broadcast without connected recipients.");
			}
			batch->recipients.reserve(_connections.size());
			results.reserve(_connections.size());
			for (const auto &[id, connection] : _connections)
			{
				if (!connection.isConnected())
				{
					if (policy == Connection::OverflowPolicy::SkipRecipient)
					{
						continue;
					}
					throw Exception("Cannot broadcast while a recipient is pending or disconnected.");
				}
				if (policy == Connection::OverflowPolicy::Reject)
				{
					NetworkInternal::OutgoingMessageValidator::validateCapacity(message, connection.maxQueuedBytes());
				}
				batch->recipients.push_back({connection, Transfer{}});
				results.emplace(id, batch->recipients.back().transfer);
			}
		}
		_impl->send(std::move(batch));
		return results;
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
	}

	void Server::_notifyReady(ConnectionID id)
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
		}
		if (connection.status() != Connection::Status::Pending)
		{
			return;
		}
		connection._setStatus(Connection::Status::Connected);
		_statusProvider.trigger(connection, Connection::Status::Connected, Connection::Error::None);
	}

	void Server::_notifyDisconnection(ConnectionID id, Connection::Error error)
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
		connection._setStatus(Connection::Status::Disconnected, error);
		_statusProvider.trigger(connection, Connection::Status::Disconnected, error);
	}
}
