#include "network/client.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"

#include <asio.hpp>

#include <array>
#include <atomic>
#include <deque>
#include <future>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace spk
{
	class Client::Impl
	{
	private:
		using TCP = asio::ip::tcp;
		using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

		Client &_owner;
		asio::io_context _context;
		std::optional<WorkGuard> _work;
		std::unique_ptr<TCP::socket> _socket;
		std::jthread _worker;
		std::atomic_bool _connected = false;
		std::array<std::byte, NetworkInternal::FrameHeaderSize> _header{};
		std::vector<std::byte> _payload;
		std::deque<std::shared_ptr<std::vector<std::byte>>> _writes;

		[[nodiscard]] bool _onWorkerThread() const noexcept
		{
			return _worker.joinable() && _worker.get_id() == std::this_thread::get_id();
		}

		void _readHeader()
		{
			asio::async_read(*_socket, asio::buffer(_header), [this](const std::error_code &error, std::size_t) {
				if (error)
				{
					_shutdown();
					return;
				}
				_handleHeader();
			});
		}

		void _handleHeader()
		{
			try
			{
				const auto header = NetworkInternal::decode(_header);
				_readPayload(header);
			}
			catch (...)
			{
				_shutdown();
			}
		}

		void _readPayload(NetworkInternal::FrameHeader header)
		{
			_payload.resize(header.payloadSize);
			if (_payload.empty())
			{
				_owner._publish(Message(header.type));
				_readHeader();
				return;
			}
			asio::async_read(*_socket, asio::buffer(_payload), [this, header](const std::error_code &error, std::size_t) {
				if (error)
				{
					_shutdown();
					return;
				}
				_owner._publish(Message(header.type, std::move(_payload)));
				_readHeader();
			});
		}

		void _queueWrite(std::shared_ptr<std::vector<std::byte>> frame)
		{
			const bool idle = _writes.empty();
			_writes.push_back(std::move(frame));
			if (idle)
			{
				_writeNext();
			}
		}

		void _writeNext()
		{
			if (_writes.empty() || !_connected)
			{
				return;
			}
			asio::async_write(*_socket, asio::buffer(*_writes.front()), [this](const std::error_code &error, std::size_t) {
				if (error)
				{
					_shutdown();
					return;
				}
				_writes.pop_front();
				_writeNext();
			});
		}

		void _closeSocket() noexcept
		{
			if (_socket == nullptr)
			{
				return;
			}
			std::error_code ignored;
			_socket->cancel(ignored);
			_socket->shutdown(TCP::socket::shutdown_both, ignored);
			_socket->close(ignored);
		}

		void _shutdown()
		{
			if (!_connected.exchange(false))
			{
				return;
			}
			_closeSocket();
			_writes.clear();
			_work.reset();
			_owner._notifyDisconnection();
		}

		template <typename TFunction>
		void _invoke(TFunction function)
		{
			if (_onWorkerThread())
			{
				function();
				return;
			}

			auto completion = std::make_shared<std::promise<void>>();
			auto future = completion->get_future();
			asio::post(_context, [function = std::move(function), completion]() mutable {
				try
				{
					function();
					completion->set_value();
				}
				catch (...)
				{
					completion->set_exception(std::current_exception());
				}
			});
			future.get();
		}

		void _joinWorker()
		{
			if (_worker.joinable() && !_onWorkerThread())
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
			}
			catch (...)
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
			_context.restart();
			_work.emplace(_context.get_executor());
			_socket = std::make_unique<TCP::socket>(_context);

			try
			{
				TCP::resolver resolver(_context);
				const auto endpoints = resolver.resolve(std::string(address), std::to_string(port));
				asio::connect(*_socket, endpoints);
			}
			catch (...)
			{
				_work.reset();
				_socket.reset();
				throw Exception("Unable to connect the network client.", std::current_exception());
			}

			_connected = true;
			_readHeader();
			_worker = std::jthread([this] {
				_context.run();
			});
			_owner._notifyConnection();
		}

		void disconnect()
		{
			if (_connected)
			{
				_invoke([this] {
					_shutdown();
				});
			}
			_joinWorker();
			_work.reset();
			_context.stop();
			_socket.reset();
			_writes.clear();
		}

		void send(const Message &message)
		{
			if (!_connected)
			{
				throw Exception("Unable to send through a disconnected network client.");
			}

			auto frame = NetworkInternal::encode(message);
			asio::post(_context, [this, frame = std::move(frame)] {
				if (_connected)
				{
					_queueWrite(std::move(frame));
				}
			});
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
