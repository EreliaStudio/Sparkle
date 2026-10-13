#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/client_handshake.hpp"
#include "network/internal/command_budget.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/handshake.hpp"
#include "network/internal/incoming_frame_validator.hpp"
#include "network/internal/outgoing_reservation.hpp"
#include "network/internal/page.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_reception.hpp"
#include "network/internal/transfer_sender.hpp"
#include "network/internal/transmission_frame.hpp"

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
#include <unordered_map>
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
		mutable std::mutex _lifecycleMutex;
		std::mutex _receiveMutex;
		std::mutex _expirationMutex;
		std::condition_variable _expirationWake;
		std::atomic_bool _connected = false;
		std::atomic_bool _active = false;
		std::chrono::milliseconds _handshakeTimeout = NetworkInternal::Handshake::DefaultTimeout;
		Connection _session;
		std::mutex _sendMutex;
		std::condition_variable _sendAvailable;
		NetworkInternal::TransferSender _transfers;
		NetworkInternal::TransferReception _reception;
		std::deque<NetworkInternal::TransmissionFrame> _responses;
		NetworkInternal::CommandBudget _outgoingBudget{64u * 1024u * 1024u};
		std::unordered_map<spk::Transfer::ID, NetworkInternal::OutgoingReservation> _outgoingReservations;
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
					NetworkInternal::IncomingFrameValidator::validate(
						NetworkInternal::read16(headerBytes.data() + 6),
						header.payloadSize,
						_negotiatedPageBytes.load(),
						_maxMessageBytes.load());
					_acceptRemoteID(header.connectionID);
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

				(void)_transfers.acknowledge(response);
				{
					const std::scoped_lock lock(_sendMutex);
					_outgoingReservations.erase(response.transferID);
				}
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
			auto reservation = _outgoingBudget.reserve(frame->size(), NetworkInternal::CommandBudget::Stage::Frame);
			{
				const std::scoped_lock lock(_sendMutex);
				if (!_connected)
				{
					return;
				}
				_responses.push_back(NetworkInternal::TransmissionFrame{std::move(frame), 0, 0, std::move(reservation)});
			}
			_sendAvailable.notify_one();
		}

		void _receivePage(std::span<const std::byte> frame)
		{
			const auto page = NetworkInternal::decodePage(frame);

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
					std::shared_ptr<NetworkInternal::CommandBudget::Lease> reservation;
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
							frame = std::move(_responses.front().bytes);
							reservation = std::move(_responses.front().reservation);
							_responses.pop_front();
						}
						else
						{
							const auto page = _transfers.nextPage(std::min(_maxPageBytes.load(), _negotiatedPageBytes.load()));
							if (page.has_value())
							{
								const auto it = _outgoingReservations.find(page->transferID);
								if (it == _outgoingReservations.end())
								{
									throw Exception("Missing client outgoing page reservation.");
								}
								frame = NetworkInternal::encodePage(*page);
								reservation = it->second.frame;
								if (page->offset + page->length == page->totalSize)
								{
									finalPage = page->transferID;
								}
							}
						}
					}
					if (frame != nullptr)
					{
						if (reservation != nullptr)
						{
							reservation->transition(NetworkInternal::CommandBudget::Stage::InFlight);
						}
						_socket.sendAll(*frame);
						if (reservation != nullptr)
						{
							reservation->transition(NetworkInternal::CommandBudget::Stage::Frame);
						}
						if (finalPage != 0)
						{
							_transfers.finalPageWritten(finalPage);
							const std::scoped_lock lock(_sendMutex);
							_outgoingReservations.erase(finalPage);
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
			{
				const std::scoped_lock lock(_lifecycleMutex, _sendMutex);
				if (!_active.exchange(false))
				{
					return;
				}
				_connected = false;
				_socket.close();
			}
			_expirationWake.notify_all();
			_owner._retireIncoming(_session);
			_sendAvailable.notify_all();
			_transfers.clear();
			{
				const std::scoped_lock lock(_receiveMutex);
				_reception.clear();
			}
			{
				const std::scoped_lock lock(_sendMutex);
				_responses.clear();
				_outgoingReservations.clear();
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

		[[nodiscard]] bool isConnected() const noexcept
		{
			const std::scoped_lock lock(_lifecycleMutex);
			return _connected.load() && _session.isConnected();
		}

		void _reclaimOutgoingReservations()
		{
			const std::scoped_lock lock(_sendMutex);
			for (auto it = _outgoingReservations.begin(); it != _outgoingReservations.end();)
			{
				if (!_transfers.isTracked(it->first))
				{
					it = _outgoingReservations.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		void _expireTransfers()
		{
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
			_reclaimOutgoingReservations();
		}

		void _expirationLoop() noexcept
		{
			try
			{
				while (_connected)
				{
					std::unique_lock lock(_expirationMutex);
					_expirationWake.wait_for(lock, std::chrono::seconds(1), [this] {
						return !_connected.load();
					});
					lock.unlock();
					if (_connected)
					{
						_expireTransfers();
					}
				}
			} catch (...)
			{
				_remoteDisconnect();
			}
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
				_expirationLoop();
			});
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
			try
			{
				{
					const std::scoped_lock lock(_lifecycleMutex);
					if (!_active || stop.stop_requested())
					{
						return;
					}
					_negotiatedPageBytes.store(result.maximumPageBytes);
					_socket = std::move(result.socket);
					_acceptRemoteID(result.id);
					_owner._acceptConnection(_session, result.id);
					_startWorkers();
				}
				if (_active)
				{
					_owner._setStatus(_session, Connection::Status::Connected);
				}
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
			{
				const std::scoped_lock lock(_lifecycleMutex);
				_session = _owner._beginConnection();
				_active = true;
			}
			try
			{
				_owner._announcePending(_session);
			} catch (...)
			{
				disconnect();
				throw;
			}
			{
				const std::scoped_lock lock(_lifecycleMutex);
				if (!_active)
				{
					return _session;
				}
				const auto deadline = std::chrono::steady_clock::now() + _handshakeTimeout;
				_connector = std::jthread([this, address = std::string(address), port, deadline](std::stop_token stop) {
					_connectWorker(address, port, deadline, stop);
				});
			}
			return _session;
		}

		void setHandshakeTimeout(std::chrono::milliseconds timeout)
		{
			if (timeout.count() <= 0)
			{
				throw Exception("Handshake timeout must be positive.");
			}
			const std::scoped_lock lock(_lifecycleMutex);
			const std::scoped_lock lock(_lifecycleMutex);
			_handshakeTimeout = timeout;
		}

		void disconnect()
		{
			bool notify = false;
			{
				const std::scoped_lock lock(_lifecycleMutex, _sendMutex);
				notify = _active.exchange(false);
				_connected = false;
				_socket.close();
			}
			_expirationWake.notify_all();
			_owner._retireIncoming(_session);
			_sendAvailable.notify_all();
			_joinWorkers();
			_socket.close();
			_transfers.clear();
			{
				const std::scoped_lock lock(_sendMutex);
				_responses.clear();
				_outgoingReservations.clear();
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
			std::unique_lock lock(_sendMutex);
			if (!_connected || _session.status() != Connection::Status::Connected)
			{
				throw Exception("Cannot send through a pending or disconnected network client.");
			}
			const auto maximum = _session.maxQueuedBytes();
			_outgoingBudget.setMaximum(maximum);
			_transfers.setMaxPendingBytes(maximum);
			auto reservation = NetworkInternal::OutgoingReservation::reserve(_outgoingBudget, message.size(), _negotiatedPageBytes.load());
			Transfer transfer;
			try
			{
				transfer = _transfers.enqueue(message, priority);
				_outgoingReservations.emplace(transfer.id(), std::move(reservation));
			} catch (...)
			{
				lock.unlock();
				if (transfer.id() != Transfer::InvalidID)
				{
					(void)_transfers.fail(transfer.id(), Transfer::Status::Rejected, Transfer::Reason::QueueOverflow);
				}
				throw;
			}
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
