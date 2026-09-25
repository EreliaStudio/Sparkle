#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/socket.hpp"

#include <array>
#include <atomic>
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
		std::jthread _worker;
		std::atomic_bool _connected = false;
		std::mutex _sendMutex;

		void _run()
		{
			try
			{
				while (_connected)
				{
					std::array<std::byte, NetworkInternal::FrameHeaderSize> bytes{};
					if (!_socket.receiveAll(bytes))
					{
						break;
					}

					const auto header = NetworkInternal::decode(bytes);
					Message::Storage payload(header.payloadSize);
					if (!payload.empty() && !_socket.receiveAll(payload))
					{
						break;
					}
					Message message(header.type, std::move(payload));
					message.setRequestID(header.requestID);
					_owner._publish(std::move(message));
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
			try
			{
				_owner._notifyDisconnection();
			} catch (...)
			{
			}
		}

		void _joinWorker()
		{
			if (_worker.joinable() && _worker.get_id() != std::this_thread::get_id())
			{
				_worker.join();
			}
		}

	public:
		explicit Impl(Client &owner) :
			_owner(owner)
		{
		}

		~Impl()
		{
			try
			{
				disconnect();
			} catch (...)
			{
			}
		}

		[[nodiscard]] bool isConnected() const noexcept
		{
			return _connected.load();
		}

		void connect(std::string_view address, std::uint16_t port)
		{
			disconnect();

			try
			{
				_socket = NetworkInternal::Socket::connectTCP(address, port);
			} catch (...)
			{
				throw Exception("Unable to connect the network client.", std::current_exception());
			}

			_connected = true;
			_worker = std::jthread([this] {
				_run();
			});

			try
			{
				_owner._notifyConnection();
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
			_joinWorker();

			if (notify)
			{
				_owner._notifyDisconnection();
			}
		}

		void send(const Message &message)
		{
			const auto frame = NetworkInternal::encode(message);
			const std::scoped_lock lock(_sendMutex);

			if (!_connected)
			{
				throw Exception("Unable to send through a disconnected network client.");
			}
			_socket.sendAll(*frame);
		}
	};

	Client::Client() :
		_impl(std::make_unique<Impl>(*this))
	{
	}

	Client::~Client() = default;

	Client::ConnectionContract Client::subscribeToConnection(ConnectionCallback callback)
	{
		return _connectionProvider.subscribe(std::move(callback));
	}

	Client::DisconnectionContract Client::subscribeToDisconnection(DisconnectionCallback callback)
	{
		return _disconnectionProvider.subscribe(std::move(callback));
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

	void Client::send(const Message &message)
	{
		_impl->send(message);
	}

	Client::MessageQueue &Client::messages() noexcept
	{
		return _messages;
	}

	void Client::_publish(Message message)
	{
		_messages.publish(std::move(message));
	}

	void Client::_notifyConnection()
	{
		_connectionProvider.trigger();
	}

	void Client::_notifyDisconnection()
	{
		_disconnectionProvider.trigger();
	}
}
