#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/client_session.hpp"
#include "network/internal/page.hpp"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace spk
{
	class Client::Impl
	{
	private:
		Client &_owner;
		mutable std::mutex _mutex;
		std::shared_ptr<NetworkInternal::ClientSession> _session;
		NetworkInternal::ClientSession::Settings _settings;
		std::uint64_t _generation = 0;

		[[nodiscard]] NetworkInternal::ClientSession::Callbacks _callbacks()
		{
			return {
				[this](Connection session, Message message) { _owner._publish(session, std::move(message)); },
				[this](Connection session) { _owner._retireIncoming(session); },
				[this](Connection session, Connection::ID id) { _owner._acceptConnection(session, id); },
				[this] { return _owner._beginConnection(); },
				[this](Connection session) { _owner._announcePending(session); },
				[this](Connection session, Connection::Status status, Connection::Error error) { _owner._setStatus(session, status, error); }};
		}

		[[nodiscard]] std::shared_ptr<NetworkInternal::ClientSession> _current() const
		{
			const std::scoped_lock lock(_mutex);
			return _session;
		}

	public:
		explicit Impl(Client &owner) : _owner(owner) {}
		~Impl() { disconnect(); }

		[[nodiscard]] bool isConnected() const noexcept
		{
			const auto session = _current();
			return session != nullptr && session->isConnected();
		}

		Connection connect(std::string_view address, std::uint16_t port, ConnectBehaviour behaviour)
		{
			if (address.empty() || port == 0)
			{
				throw Exception("Client::connect requires a nonempty address and nonzero port.");
			}
			if (behaviour != ConnectBehaviour::Throw && behaviour != ConnectBehaviour::DisconnectPrevious)
			{
				throw Exception("Unsupported client connection behavior.");
			}
			std::shared_ptr<NetworkInternal::ClientSession> previous;
			std::uint64_t generation;
			{
				const std::scoped_lock lock(_mutex);
				previous = _session;
				if (previous != nullptr && previous->isWorkerThread())
				{
					throw Exception("Cannot begin a client connection from a transport worker callback.");
				}
				if (behaviour == ConnectBehaviour::Throw && previous != nullptr && previous->active())
				{
					throw Exception("Disconnect the current network session before connecting again.");
				}
				generation = ++_generation;
				_session.reset();
			}
			if (previous != nullptr)
			{
				previous->disconnect();
			}
			std::shared_ptr<NetworkInternal::ClientSession> session;
			{
				const std::scoped_lock lock(_mutex);
				if (_generation != generation)
				{
					throw Exception("Client connection attempt was superseded.");
				}
				session = std::make_shared<NetworkInternal::ClientSession>(_callbacks(), _settings);
				_session = session;
			}
			return session->connect(address, port);
		}

		void disconnect()
		{
			const auto session = _current();
			if (session != nullptr)
			{
				session->disconnect();
			}
		}

		void setHandshakeTimeout(std::chrono::milliseconds timeout)
		{
			if (timeout.count() <= 0) throw Exception("Handshake timeout must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setHandshakeTimeout(timeout);
			_settings.handshakeTimeout = timeout;
		}
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout)
		{
			if (timeout.count() <= 0) throw Exception("Incoming transfer timeout must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setIncomingTransferTimeout(timeout);
			_settings.incomingTransferTimeout = timeout;
		}
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
		{
			if (timeout.count() <= 0) throw Exception("Acknowledgment timeout must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setAcknowledgmentTimeout(timeout);
			_settings.acknowledgmentTimeout = timeout;
		}
		void setMaxMessageBytes(std::size_t bytes)
		{
			if (bytes == 0) throw Exception("Maximum network message size must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setMaxMessageBytes(bytes);
			_settings.maxMessageBytes = bytes;
		}
		void setMaxPageBytes(std::size_t bytes)
		{
			if (bytes == 0 || bytes > NetworkInternal::MaximumSupportedPageBytes) throw Exception("Network page size must be within the supported protocol limit.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setMaxPageBytes(bytes);
			_settings.maxPageBytes = bytes;
		}
		void setMaxIncomingBytes(std::size_t bytes)
		{
			if (bytes == 0) throw Exception("Maximum incomplete incoming bytes must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setMaxIncomingBytes(bytes);
			_settings.maxIncomingBytes = bytes;
		}
		void setPageFailureLimits(PageFailureLimits limits)
		{
			if (!limits.valid()) throw Exception("Network failure limits must be positive.");
			const std::scoped_lock lock(_mutex);
			if (_session != nullptr) _session->setPageFailureLimits(limits);
			_settings.pageFailureLimits = limits;
		}
		Transfer send(const Message &message, Message::Priority priority)
		{
			auto session = _current();
			if (session == nullptr) throw Exception("Cannot send through a pending or disconnected network client.");
			return session->send(message, priority);
		}
	};

	Client::Client() :
		_impl(std::make_unique<Impl>(*this))
	{
	}

	Client::~Client()
	{
		disconnect();
	}

	Client::StatusContract Client::subscribeToConnectionStatusChange(StatusCallback callback)
	{
		return _statusProvider.subscribe(std::move(callback));
	}

	Connection Client::connection() const
	{
		const std::scoped_lock lock(_connectionMutex);
		return _connection;
	}

	bool Client::isConnected() const noexcept
	{
		return _impl->isConnected();
	}

	Connection Client::connect(std::string_view address, std::uint16_t port, ConnectBehaviour behaviour, ConnectAttemptBehaviour attempt)
	{
		if (attempt != ConnectAttemptBehaviour::ReturnPending)
		{
			throw Exception("Unsupported client connection attempt behaviour.");
		}
		return _impl->connect(address, port, behaviour);
	}

	void Client::setHandshakeTimeout(std::chrono::milliseconds timeout, HandshakeTimeoutBehaviour behaviour)
	{
		if (behaviour != HandshakeTimeoutBehaviour::FutureAttemptsOnly)
		{
			throw Exception("Unsupported client handshake timeout behaviour.");
		}
		_impl->setHandshakeTimeout(timeout);
	}

	void Client::disconnect()
	{
		_impl->disconnect();
	}

	void Client::setIncomingTransferTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setIncomingTransferTimeout(timeout);
	}

	void Client::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setAcknowledgmentTimeout(timeout);
	}

	void Client::setMaxMessageBytes(std::size_t bytes)
	{
		_impl->setMaxMessageBytes(bytes);
	}
	void Client::setMaxPageBytes(std::size_t bytes, PageSizeBehaviour behaviour)
	{
		if (behaviour != PageSizeBehaviour::RespectNegotiatedLimit)
		{
			throw Exception("Unsupported client page size behaviour.");
		}
		_impl->setMaxPageBytes(bytes);
	}

	void Client::setMaxIncomingBytes(std::size_t bytes, IncomingBudgetBehaviour behaviour)
	{
		if (behaviour != IncomingBudgetBehaviour::AccountPayloadAndMetadataPreservingAccepted)
		{
			throw Exception("Unsupported client incoming budget behaviour.");
		}
		_impl->setMaxIncomingBytes(bytes);
	}
	void Client::setPageFailureLimits(PageFailureLimits limits)
	{
		_impl->setPageFailureLimits(limits);
	}

	Transfer Client::send(const Message &message, Message::Priority priority, SendBehaviour behaviour)
	{
		if (behaviour != SendBehaviour::SynchronousAdmission)
		{
			throw Exception("Unsupported client send behaviour.");
		}
		try
		{
			return _impl->send(message, priority);
		} catch (const Exception &)
		{
			throw;
		} catch (...)
		{
			throw Exception("Network client send admission failed.", std::current_exception());
		}
	}

	Client::MessageQueue &Client::messages() noexcept
	{
		return _messages;
	}

	Client::MessageDispatcher &Client::messageDispatcher() noexcept
	{
		return _messageDispatcher;
	}

	void Client::treatMessages(MessageDispatchBehaviour behaviour)
	{
		if (behaviour != MessageDispatchBehaviour::CallingThread)
		{
			throw Exception("Unsupported client message dispatch behaviour.");
		}
		_messageDispatcher.treatMessages(_messages);
	}

	void Client::_publish(Connection session, Message message)
	{
		const std::scoped_lock lock(_connectionMutex);
		if (_connection._data == session._data && _acceptIncoming)
		{
			_messages.publish(std::move(message));
		}
	}

	void Client::_retireIncoming(Connection session)
	{
		std::vector<Message> discarded;
		const std::scoped_lock lock(_connectionMutex);
		if (_connection._data == session._data)
		{
			_acceptIncoming = false;
			_messages.drain(discarded);
		}
	}

	Connection Client::_beginConnection()
	{
		std::vector<Message> stale;
		Connection connection;
		{
			const std::scoped_lock lock(_connectionMutex);
			_messages.drain(stale);
			_connection = connection;
			_acceptIncoming = true;
		}
		connection._setStatus(Connection::Status::Pending);
		return connection;
	}

	void Client::_announcePending(Connection connection)
	{
		_statusProvider.trigger(connection, Connection::Status::Pending, Connection::Error::None);
	}

	void Client::_acceptConnection(Connection connection, Connection::ID id)
	{
		const std::scoped_lock lock(_connectionMutex);
		if (_connection._data == connection._data && connection.status() == Connection::Status::Pending)
		{
			connection._assignID(id);
		}
	}

	void Client::_setStatus(Connection connection, Connection::Status status, Connection::Error error)
	{
		{
			const std::scoped_lock lock(_connectionMutex);
			if (_connection._data != connection._data || connection.status() == status)
			{
				return;
			}
		}
		if (connection._setStatus(status, error) && connection.status() == status)
		{
			_statusProvider.trigger(connection, status, error);
		}
	}
}
