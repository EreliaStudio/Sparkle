#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/page.hpp"
#include "network/internal/transfer_reception.hpp"
#include "network/internal/transfer_sender.hpp"
#include "network/internal/socket.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
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
		std::jthread _reader;
		std::jthread _sender;
		std::jthread _expirationWorker;
		std::mutex _receiveMutex;
		std::atomic_bool _connected = false;
		Connection _session;
		std::mutex _sendMutex;
		std::condition_variable _sendAvailable;
		NetworkInternal::TransferSender _transfers;
		NetworkInternal::TransferReception _reception;
		std::deque<std::shared_ptr<std::vector<std::byte>>> _responses;
		std::atomic<Connection::ID> _remoteID = Connection::InvalidID;

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
			Message::Writer writer(header.type);
			writer.setRequestID(header.requestID);
			writer.setConnectionID(header.connectionID);
			writer.resize(header.payloadSize);
			std::copy(frame.begin() + NetworkInternal::FrameHeaderSize, frame.end(), writer.data().begin());
			_owner._publish(std::move(writer).build());
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
			_acceptRemoteID(page.header.connectionID);
			NetworkInternal::TransferReception::Result received;
			{
				const std::scoped_lock lock(_receiveMutex);
				received = _reception.accept(page, page.header.connectionID);
			}
			if (received.message.has_value())
			{
				_owner._publish(std::move(*received.message));
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
						const auto page = _transfers.nextPage(NetworkInternal::DefaultMaximumPageBytes);
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

		void _remoteDisconnect() noexcept
		{
			if (!_connected.exchange(false))
			{
				return;
			}
			_socket.close();
			_sendAvailable.notify_all();
			_transfers.clear();
			try
			{
				_owner._setStatus(_session, Connection::Status::Disconnected, Connection::Error::TransportFailure);
			} catch (...)
			{
			}
		}

		void _joinWorkers()
		{
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
			return _connected.load();
		}

		void connect(std::string_view address, std::uint16_t port)
		{
			disconnect();
			_session = _owner._beginConnection();
			try
			{
				_socket = NetworkInternal::Socket::connectTCP(address, port);
			} catch (...)
			{
				_owner._setStatus(_session, Connection::Status::Disconnected, Connection::Error::Refused);
				throw Exception("Unable to connect the network client.", std::current_exception());
			}
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
			try
			{
				_owner._setStatus(_session, Connection::Status::Connected);
			} catch (...)
			{
				disconnect();
				throw;
			}
		}

		void disconnect()
		{
			const bool notify = _connected.exchange(false);
			_socket.close();
			_sendAvailable.notify_all();
			_joinWorkers();
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

		void setPageFailureLimits(PageFailureLimits limits)
		{
			_reassembler.configureFailureLimits(limits);
		}

		void send(const Message &message, Message::Priority priority)
		{
			const std::scoped_lock lock(_sendMutex);
			if (!_connected)
			{
				throw Exception("Unable to send through a disconnected network client.");
			}
			(void)_transfers.enqueue(message, priority);
			_sendAvailable.notify_one();
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

	void Client::connect(std::string_view address, std::uint16_t port)
	{
		_impl->connect(address, port);
	}

	void Client::disconnect()
	{
		_impl->disconnect();
	}

	void Client::setPageFailureLimits(PageFailureLimits limits)
	{
		_impl->setPageFailureLimits(limits);
	}

	void Client::send(const Message &message, Message::Priority priority)
	{
		_impl->send(message, priority);
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

	void Client::_publish(Message message)
	{
		_messages.publish(std::move(message));
	}

	Connection Client::_beginConnection()
	{
		Connection connection;
		{
			const std::scoped_lock lock(_connectionMutex);
			_connection = connection;
		}
		connection._setStatus(Connection::Status::Pending);
		_statusProvider.trigger(connection, Connection::Status::Pending, Connection::Error::None);
		return connection;
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
