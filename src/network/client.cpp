#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/page.hpp"
#include "network/internal/page_reassembler.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/socket.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
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
		std::atomic_bool _connected = false;
		Connection _session;
		std::mutex _sendMutex;
		std::condition_variable _sendAvailable;
		NetworkInternal::PageScheduler _scheduler;
		NetworkInternal::PageReassembler _reassembler;

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
				_reassembler.expire();
				try
				{
					auto message = _reassembler.accept(NetworkInternal::decodePage(frame));
					if (message.has_value())
					{
						_owner._publish(std::move(*message));
					}
				}
				catch (const NetworkInternal::PageReassemblyError &)
				{
					if (_reassembler.failureLimitReached())
					{
						throw;
					}
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
			_owner._publish(std::move(writer).build());
			_reassembler.successfulMessage();
		}

		void _sendLoop()
		{
			try
			{
				while (_connected)
				{
					{
						std::unique_lock lock(_sendMutex);
						_sendAvailable.wait(lock, [this] {
							return !_connected || _scheduler.pendingTransfers() != 0;
						});
					}
					if (!_connected)
					{
						break;
					}
					const auto page = _scheduler.nextPage(DefaultMaximumPageBytes);
					if (page.has_value())
					{
						_socket.sendAll(*NetworkInternal::encodePage(*page));
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
			_scheduler.clear();
			_reassembler.clear();
			if (notify)
			{
				_owner._setStatus(_session, Connection::Status::Disconnected);
			}
		}

		void send(const Message &message, Message::Priority priority)
		{
			const std::scoped_lock lock(_sendMutex);
			if (!_connected)
			{
				throw Exception("Unable to send through a disconnected network client.");
			}
			(void)_scheduler.enqueue(message, priority);
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
