#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/client_handshake.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/page.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_reception.hpp"
#include "network/internal/transfer_sender.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace spk
{
	class Client::Impl
	{
	private:
		Client &_owner;
		NetworkInternal::Socket _socket;
		std::jthread _connector;
		std::jthread _reader;
		std::jthread _sender;
		std::jthread _expirationWorker;
		std::mutex _receiveMutex;
		std::atomic_bool _connected = false;
		std::atomic_bool _active = false;
		std::chrono::milliseconds _handshakeTimeout = NetworkInternal::Handshake::DefaultTimeout;
		Connection _session;
		std::mutex _sendMutex;
		std::condition_variable _sendAvailable;
		NetworkInternal::TransferSender _transfers;
		NetworkInternal::TransferReception _reception;
		std::deque<std::shared_ptr<std::vector<std::byte>>> _responses;
		std::atomic<Connection::ID> _remoteID = Connection::InvalidID;
		std::atomic_size_t _maxMessageBytes = 128u * 1024u * 1024u;
		std::atomic_size_t _maxIncomingBytes = 512u * 1024u * 1024u;
		std::atomic_size_t _maxPageBytes = NetworkInternal::DefaultMaximumPageBytes;
		std::atomic_size_t _negotiatedPageBytes = NetworkInternal::DefaultMaximumPageBytes;

		void _run()
		{
			try
			{
				while (_connected)
				{
					std::array<std::byte, NetworkInternal::FrameHeaderSize> headerBytes{};
					if (!_socket.receiveAll(headerBytes))
					{
						break;
					}
					const auto header = NetworkInternal::decode(headerBytes);
					if (NetworkInternal::read16(headerBytes.data() + 6) == 0 && header.payloadSize > _maxMessageBytes.load())
					{
						throw Exception("Incoming logical message exceeds the configured maximum.");
					}
					std::vector<std::byte> frame(NetworkInternal::FrameHeaderSize + header.payloadSize);
					std::copy(headerBytes.begin(), headerBytes.end(), frame.begin());
					if (header.payloadSize != 0 &&
						!_socket.receiveAll(std::span<std::byte>(frame).subspan(NetworkInternal::FrameHeaderSize)))
					{
						break;
					}
					_receive(header, frame);
				}
			} catch (...)
			{
			}
			_remoteDisconnect();
		}

		void _receive(const NetworkInternal::FrameHeader &header, std::span<const std::byte> frame)
		{
			const auto kind = NetworkInternal::read16(frame.data() + 6);
			if (kind == NetworkInternal::PageFrameKind)
			{
				_receivePage(frame);
				return;
			}
			if (kind == NetworkInternal::TransferResponse::FrameKind)
			{
				const auto response = NetworkInternal::TransferResponse::decode(frame);
				_acceptRemoteID(response.connectionID);
				(void)_transfers.acknowledge(response);
				return;
			}
			if (kind != 0)
			{
				throw Exception("Unknown network frame kind.");
			}
			_acceptRemoteID(header.connectionID);
			Message::Writer writer(header.type);
			writer.setRequestID(header.requestID);
			writer.setConnectionID(header.connectionID);
			writer.resize(header.payloadSize);
			std::copy(frame.begin() + NetworkInternal::FrameHeaderSize, frame.end(), writer.data().begin());
			_owner._publish(_session, std::move(writer).build());
			{
				const std::scoped_lock lock(_receiveMutex);
				_reception.successfulMessage();
			}
		}

		void _acceptRemoteID(Connection::ID id)
		{
			if (id == Connection::InvalidID)
			{
				throw Exception("Incoming message has no server-assigned connection ID.");
			}
			Connection::ID expected = Connection::InvalidID;
			if (!_remoteID.compare_exchange_strong(expected, id) && expected != id)
			{
				throw Exception("Unexpected connection ID in incoming message.");
			}
		}

		void _queueResponse(const NetworkInternal::TransferResponse &response)
		{
			auto frame = response.encode();
			{
				const std::scoped_lock lock(_sendMutex);
				if (!_connected)
				{
					return;
				}
				_responses.push_back(std::move(frame));
			}
			_sendAvailable.notify_one();
		}

		void _receivePage(std::span<const std::byte> frame)
		{
			const auto page = NetworkInternal::decodePage(frame);
			if (page.data.size() > _negotiatedPageBytes.load())
			{
				throw Exception("Network page exceeds the negotiated maximum payload size.");
			}
			_acceptRemoteID(page.header.connectionID);
			NetworkInternal::TransferReception::Result received;
			{
				const std::scoped_lock lock(_receiveMutex);
				received = _reception.accept(page, page.header.connectionID);
			}
			if (received.message.has_value())
			{
				_owner._publish(_session, std::move(*received.message));
			}
			if (received.response.has_value())
			{
				_queueResponse(*received.response);
			}
		}

		void _sendLoop()
		{
			try
			{
				while (_connected)
				{
					std::shared_ptr<std::vector<std::byte>> frame;
					std::uint64_t finalPage = 0;
					{
						std::unique_lock lock(_sendMutex);
						_sendAvailable.wait(lock, [this] {
							return !_connected || !_responses.empty() || _transfers.pendingTransfers() != 0;
						});
						if (!_connected)
						{
							break;
						}
						if (!_responses.empty())
						{
							frame = std::move(_responses.front());
							_responses.pop_front();
						}
					}
					if (frame == nullptr)
					{
						const auto page = _transfers.nextPage(std::min(_maxPageBytes.load(), _negotiatedPageBytes.load()));
						if (page.has_value())
						{
							frame = NetworkInternal::encodePage(*page);
							if (page->offset + page->length == page->totalSize)
							{
								finalPage = page->transferID;
							}
						}
					}
					if (frame != nullptr)
					{
						_socket.sendAll(*frame);
						if (finalPage != 0)
						{
							_transfers.finalPageWritten(finalPage);
						}
					}
				}
			} catch (...)
			{
			}
			_remoteDisconnect();
		}

		void _remoteDisconnect(Connection::Error error = Connection::Error::TransportFailure) noexcept
		{
			if (!_active.exchange(false))
			{
				return;
			}
			_connected = false;
			_owner._retireIncoming(_session);
			_socket.close();
			_sendAvailable.notify_all();
			_transfers.clear();
			{
				const std::scoped_lock lock(_receiveMutex);
				_reception.clear();
			}
			{
				const std::scoped_lock lock(_sendMutex);
				_responses.clear();
			}
			try
			{
				_owner._setStatus(_session, Connection::Status::Disconnected, error);
			} catch (...)
			{
			}
		}

		void _joinWorkers()
		{
			if (_connector.joinable() && _connector.get_id() != std::this_thread::get_id())
			{
				_connector.request_stop();
				_connector.join();
			}
			if (_reader.joinable() && _reader.get_id() != std::this_thread::get_id())
			{
				_reader.join();
			}
			if (_sender.joinable() && _sender.get_id() != std::this_thread::get_id())
			{
				_sender.join();
			}
			if (_expirationWorker.joinable() && _expirationWorker.get_id() != std::this_thread::get_id())
			{
				_expirationWorker.join();
			}
		}

	public:
		explicit Impl(Client &owner) :
			_owner(owner)
		{
		}

		~Impl()
		{
			disconnect();
		}

		[[nodiscard]] bool isConnected() const noexcept
		{
			return _connected.load() && _session.isConnected();
		}

		void _startWorkers()
		{
			_connected = true;
			_reader = std::jthread([this] {
				_run();
			});
			_sender = std::jthread([this] {
				_sendLoop();
			});
			_expirationWorker = std::jthread([this] {
				while (_connected)
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::vector<NetworkInternal::TransferResponse> responses;
					{
						const std::scoped_lock lock(_receiveMutex);
						responses = _reception.expire(_remoteID.load());
					}
					for (const auto &response : responses)
					{
						_queueResponse(response);
					}
					_transfers.expire();
				}
			});
			_owner._setStatus(_session, Connection::Status::Connected);
		}

		void _connectWorker(std::string address, std::uint16_t port, std::chrono::steady_clock::time_point deadline, std::stop_token stop)
		{
			NetworkInternal::ClientHandshake handshake(deadline, stop, static_cast<std::uint32_t>(_maxPageBytes.load()));
			auto result = handshake.connect(address, port);
			if (result.error != Connection::Error::None)
			{
				_remoteDisconnect(result.error);
				return;
			}
			if (!_active || stop.stop_requested())
			{
				return;
			}
			try
			{
				_negotiatedPageBytes.store(result.maximumPageBytes);
				_socket = std::move(result.socket);
				_acceptRemoteID(result.id);
				_owner._acceptConnection(_session, result.id);
				_startWorkers();
			} catch (...)
			{
				_remoteDisconnect();
			}
		}

		Connection connect(std::string_view address, std::uint16_t port)
		{
			if (address.empty() || port == 0)
			{
				throw Exception("Client::connect requires a nonempty address and nonzero port.");
			}
			if (_active.load())
			{
				throw Exception("Disconnect the current network session before connecting again.");
			}
			disconnect();
			_session = _owner._beginConnection();
			_active = true;
			const auto deadline = std::chrono::steady_clock::now() + _handshakeTimeout;
			_connector = std::jthread([this, address = std::string(address), port, deadline](std::stop_token stop) {
				_connectWorker(address, port, deadline, stop);
			});
			return _session;
		}

		void setHandshakeTimeout(std::chrono::milliseconds timeout)
		{
			if (timeout.count() <= 0)
			{
				throw Exception("Handshake timeout must be positive.");
			}
			_handshakeTimeout = timeout;
		}

		void disconnect()
		{
			const bool notify = _active.exchange(false);
			_connected = false;
			_owner._retireIncoming(_session);
			_socket.close();
			_sendAvailable.notify_all();
			_joinWorkers();
			_socket.close();
			_transfers.clear();
			{
				const std::scoped_lock lock(_sendMutex);
				_responses.clear();
			}
			{
				const std::scoped_lock lock(_receiveMutex);
				_reception.clear();
			}
			_remoteID = Connection::InvalidID;
			if (notify)
			{
				_owner._setStatus(_session, Connection::Status::Disconnected);
			}
		}

		void setIncomingTransferTimeout(std::chrono::milliseconds timeout)
		{
			const std::scoped_lock lock(_receiveMutex);
			_reception.setInactivityTimeout(timeout);
		}

		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
		{
			_transfers.setAcknowledgmentTimeout(timeout);
		}

		void setMaxMessageBytes(std::size_t bytes)
		{
			if (bytes == 0)
			{
				throw Exception("Maximum network message size must be positive.");
			}
			const std::scoped_lock lock(_receiveMutex, _sendMutex);
			_reception.setLimits(bytes, _maxIncomingBytes.load());
			_transfers.setMaxMessageBytes(bytes);
			_maxMessageBytes.store(bytes);
		}
		void setMaxPageBytes(std::size_t bytes)
		{
			if (bytes == 0 || bytes > NetworkInternal::MaximumSupportedPageBytes)
			{
				throw Exception("Network page size must be within the supported protocol limit.");
			}
			_maxPageBytes.store(bytes);
		}

		void setMaxIncomingBytes(std::size_t bytes)
		{
			const std::scoped_lock lock(_receiveMutex);
			_reception.setLimits(_maxMessageBytes.load(), bytes);
			_maxIncomingBytes.store(bytes);
		}
		void setPageFailureLimits(PageFailureLimits limits)
		{
			const std::scoped_lock lock(_receiveMutex);
			_reception.configureFailureLimits(limits);
		}

		Transfer send(const Message &message, Message::Priority priority)
		{
			const std::scoped_lock lock(_sendMutex);
			if (!_connected || _session.status() != Connection::Status::Connected)
			{
				throw Exception("Cannot send through a pending or disconnected network client.");
			}
			_transfers.setMaxPendingBytes(_session.maxQueuedBytes());
			Transfer transfer = _transfers.enqueue(message, priority);
			_sendAvailable.notify_one();
			return transfer;
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

	Connection Client::connect(std::string_view address, std::uint16_t port)
	{
		return _impl->connect(address, port);
	}

	void Client::setHandshakeTimeout(std::chrono::milliseconds timeout)
	{
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
	void Client::setMaxPageBytes(std::size_t bytes)
	{
		_impl->setMaxPageBytes(bytes);
	}

	void Client::setMaxIncomingBytes(std::size_t bytes)
	{
		_impl->setMaxIncomingBytes(bytes);
	}
	void Client::setPageFailureLimits(PageFailureLimits limits)
	{
		_impl->setPageFailureLimits(limits);
	}

	Transfer Client::send(const Message &message, Message::Priority priority)
	{
		return _impl->send(message, priority);
	}

	Client::MessageQueue &Client::messages() noexcept
	{
		return _messages;
	}

	Client::MessageDispatcher &Client::messageDispatcher() noexcept
	{
		return _messageDispatcher;
	}

	void Client::treatMessages()
	{
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
		_statusProvider.trigger(connection, Connection::Status::Pending, Connection::Error::None);
		return connection;
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
		connection._setStatus(status, error);
		_statusProvider.trigger(connection, status, error);
	}
}
