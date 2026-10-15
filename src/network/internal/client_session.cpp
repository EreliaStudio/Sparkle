#include "network/internal/client_session.hpp"
#include "network/internal/client_reception.hpp"
#include "network/internal/client_transmission.hpp"

#include "exception.hpp"
#include "network/internal/client_handshake.hpp"
#include "network/internal/command_budget.hpp"
#include "network/internal/exception_reporting.hpp"
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

namespace spk::NetworkInternal
{
	class ClientSession::Impl
	{
	private:
		Callbacks _callbacks;
		NetworkInternal::Socket _socket;
		std::jthread _connector;
		std::jthread _reader;
		std::jthread _sender;
		std::jthread _expirationWorker;
		mutable std::mutex _lifecycleMutex;
		std::mutex _expirationMutex;
		std::condition_variable _expirationWake;
		std::atomic_bool _connected = false;
		std::atomic_bool _active = false;
		std::chrono::milliseconds _handshakeTimeout = NetworkInternal::Handshake::DefaultTimeout;
		Connection _session;
		ClientReception _reception;
		ClientTransmission _transmission;
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
				if (_connected)
				{
					NetworkInternal::reportCurrentException("Network client receive worker failed.");
				}
			}
			_remoteDisconnect();
		}

		void _receive(const NetworkInternal::FrameHeader &header, std::span<const std::byte> frame)
		{
			auto received = _reception.accept(header, frame);
			if (received.acknowledgment.has_value())
			{
				_transmission.acknowledge(*received.acknowledgment);
			}
			if (received.message.has_value())
			{
				_callbacks.publish(_session, std::move(*received.message));
			}
			if (received.response.has_value())
			{
				_queueResponse(*received.response);
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
			if (_connected)
			{
				_transmission.queueResponse(response);
			}
		}

		void _sendLoop()
		{
			try
			{
				while (_connected)
				{
					auto frame = _transmission.nextFrame(_connected, std::min(_maxPageBytes.load(), _negotiatedPageBytes.load()));
					if (!frame.has_value())
					{
						break;
					}
					if (frame->reservation != nullptr)
					{
						frame->reservation->transition(NetworkInternal::CommandBudget::Stage::InFlight);
					}
					_socket.sendAll(*frame->bytes);
					if (frame->reservation != nullptr)
					{
						frame->reservation->transition(NetworkInternal::CommandBudget::Stage::Frame);
					}
					_transmission.frameWritten(frame->finalPage);
				}
			} catch (...)
			{
				if (_connected)
				{
					NetworkInternal::reportCurrentException("Network client send worker failed.");
				}
			}
			_remoteDisconnect();
		}

		void _remoteDisconnect(Connection::Error error = Connection::Error::TransportFailure) noexcept
		{
			{
				const std::scoped_lock lock(_lifecycleMutex);
				if (!_active.exchange(false))
				{
					return;
				}
				_connected = false;
				_socket.close();
			}
			_expirationWake.notify_all();
			_callbacks.retireIncoming(_session);
			_transmission.wake();
			_transmission.clear();
			_reception.clear();
			try
			{
				_callbacks.setStatus(_session, Connection::Status::Disconnected, error);
			} catch (...)
			{
				NetworkInternal::reportCurrentException("Network client Disconnected callback failed.");
			}
		}

		[[nodiscard]] bool _onWorkerThread() const noexcept
		{
			const auto self = std::this_thread::get_id();
			if (_connector.joinable() && _connector.get_id() == self)
			{
				return true;
			}
			if (_reader.joinable() && _reader.get_id() == self)
			{
				return true;
			}
			if (_sender.joinable() && _sender.get_id() == self)
			{
				return true;
			}
			return _expirationWorker.joinable() && _expirationWorker.get_id() == self;
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
		explicit Impl(Callbacks callbacks, const Settings &settings) :
			_callbacks(std::move(callbacks))
		{
			_handshakeTimeout = settings.handshakeTimeout;
			_maxMessageBytes = settings.maxMessageBytes;
			_maxIncomingBytes = settings.maxIncomingBytes;
			_maxPageBytes = settings.maxPageBytes;
			_reception.setLimits(settings.maxMessageBytes, settings.maxIncomingBytes);
			_reception.setInactivityTimeout(settings.incomingTransferTimeout);
			_reception.configureFailureLimits(settings.pageFailureLimits);
			_transmission.setMaxMessageBytes(settings.maxMessageBytes);
			_transmission.setAcknowledgmentTimeout(settings.acknowledgmentTimeout);
		}

		[[nodiscard]] bool active() const noexcept
		{
			return _active.load();
		}

		[[nodiscard]] bool onWorkerThread() const noexcept
		{
			return _onWorkerThread();
		}

		[[nodiscard]] bool isConnected() const noexcept
		{
			const std::scoped_lock lock(_lifecycleMutex);
			return _connected.load() && _session.isConnected();
		}

		void _expireTransfers()
		{
			for (const auto &response : _reception.expire(_remoteID.load()))
			{
				_queueResponse(response);
			}
			_transmission.expire();
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
				NetworkInternal::reportCurrentException("Network client transfer expiration failed.");
				_remoteDisconnect();
			}
		}

		void _startWorkers()
		{
			_transmission.start();
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
					_callbacks.acceptConnection(_session, result.id);
					_startWorkers();
				}
				if (_active)
				{
					_callbacks.setStatus(_session, Connection::Status::Connected, Connection::Error::None);
				}
			} catch (...)
			{
				NetworkInternal::reportCurrentException("Network client connection startup or Connected callback failed.");
				_remoteDisconnect();
			}
		}

		Connection connect(std::string_view address, std::uint16_t port, Client::ConnectBehaviour behaviour)
		{
			if (address.empty() || port == 0)
			{
				throw Exception("Client::connect requires a nonempty address and nonzero port.");
			}
			if (_onWorkerThread())
			{
				throw Exception("Cannot begin a client connection from a transport worker callback.");
			}
			switch (behaviour)
			{
			case Client::ConnectBehaviour::Throw:
				if (_active.load())
				{
					throw Exception("Disconnect the current network session before connecting again.");
				}
				break;
			case Client::ConnectBehaviour::DisconnectPrevious:
				break;
			default:
				throw Exception("Unsupported client connection behavior.");
			}
			disconnect();
			{
				const std::scoped_lock lock(_lifecycleMutex);
				_session = _callbacks.beginConnection();
				_active = true;
			}
			try
			{
				_callbacks.announcePending(_session);
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
			_handshakeTimeout = timeout;
		}

		void disconnect()
		{
			bool notify = false;
			{
				const std::scoped_lock lock(_lifecycleMutex);
				notify = _active.exchange(false);
				_connected = false;
				_socket.close();
			}
			_expirationWake.notify_all();
			if (notify)
			{
				_callbacks.retireIncoming(_session);
			}
			_transmission.wake();
			_joinWorkers();
			_socket.close();
			_transmission.clear();
			_reception.clear();
			_remoteID = Connection::InvalidID;
			if (notify)
			{
				_callbacks.setStatus(_session, Connection::Status::Disconnected, Connection::Error::None);
			}
		}

		void setIncomingTransferTimeout(std::chrono::milliseconds timeout)
		{
			_reception.setInactivityTimeout(timeout);
		}

		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
		{
			_transmission.setAcknowledgmentTimeout(timeout);
		}

		void setMaxMessageBytes(std::size_t bytes)
		{
			if (bytes == 0)
			{
				throw Exception("Maximum network message size must be positive.");
			}
			_reception.setLimits(bytes, _maxIncomingBytes.load());
			_transmission.setMaxMessageBytes(bytes);
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
			_reception.setLimits(_maxMessageBytes.load(), bytes);
			_maxIncomingBytes.store(bytes);
		}

		void setPageFailureLimits(PageFailureLimits limits)
		{
			_reception.configureFailureLimits(limits);
		}

		Transfer send(const Message &message, Message::Priority priority)
		{
			{
				const std::scoped_lock lock(_lifecycleMutex);
				if (!_connected || _session.status() != Connection::Status::Connected)
				{
					throw Exception("Cannot send through a pending or disconnected network client.");
				}
			}
			return _transmission.send(message, priority, _session.maxQueuedBytes(), _negotiatedPageBytes.load());
		}

	};

	ClientSession::ClientSession(Callbacks callbacks, const Settings &settings) :
		_impl(std::make_unique<Impl>(std::move(callbacks), settings))
	{
	}

	ClientSession::~ClientSession()
	{
		_impl->disconnect();
	}

	bool ClientSession::active() const noexcept
	{
		return _impl->active();
	}
	bool ClientSession::isWorkerThread() const noexcept
	{
		return _impl->onWorkerThread();
	}
	bool ClientSession::isConnected() const noexcept
	{
		return _impl->isConnected();
	}
	Connection ClientSession::connect(std::string_view address, std::uint16_t port)
	{
		return _impl->connect(address, port, Client::ConnectBehaviour::Throw);
	}
	void ClientSession::disconnect()
	{
		_impl->disconnect();
	}
	void ClientSession::setHandshakeTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setHandshakeTimeout(timeout);
	}
	void ClientSession::setIncomingTransferTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setIncomingTransferTimeout(timeout);
	}
	void ClientSession::setAcknowledgmentTimeout(std::chrono::milliseconds timeout)
	{
		_impl->setAcknowledgmentTimeout(timeout);
	}
	void ClientSession::setMaxMessageBytes(std::size_t bytes)
	{
		_impl->setMaxMessageBytes(bytes);
	}
	void ClientSession::setMaxPageBytes(std::size_t bytes)
	{
		_impl->setMaxPageBytes(bytes);
	}
	void ClientSession::setMaxIncomingBytes(std::size_t bytes)
	{
		_impl->setMaxIncomingBytes(bytes);
	}
	void ClientSession::setPageFailureLimits(PageFailureLimits limits)
	{
		_impl->setPageFailureLimits(limits);
	}
	Transfer ClientSession::send(const Message &message, Message::Priority priority)
	{
		return _impl->send(message, priority);
	}
}
