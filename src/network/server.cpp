#include "network/server.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"

#include <asio.hpp>

#include <array>
#include <atomic>
#include <deque>
#include <future>
#include <optional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	class Server::Impl
	{
	private:
		using TCP = asio::ip::tcp;
		using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;
		using Frame = std::shared_ptr<std::vector<std::byte>>;

		struct Session
		{
			ConnectionID id;
			TCP::socket socket;
			std::array<std::byte, NetworkInternal::FrameHeaderSize> header{};
			std::vector<std::byte> payload;
			std::deque<Frame> writes;

			Session(ConnectionID id, TCP::socket socket) :
				id(id),
				socket(std::move(socket))
			{
			}
		};

		Server &_owner;
		asio::io_context _context;
		std::optional<WorkGuard> _work;
		std::unique_ptr<TCP::acceptor> _acceptor;
		std::jthread _worker;
		std::atomic_bool _running = false;
		std::atomic_uint16_t _port = 0;
		ConnectionID _nextConnectionID = 1;
		std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;

		[[nodiscard]] bool _onWorkerThread() const noexcept
		{
			return _worker.joinable() && _worker.get_id() == std::this_thread::get_id();
		}

		void _accept()
		{
			_acceptor->async_accept([this](const std::error_code &error, TCP::socket socket) {
				if (!error && _running)
				{
					_addSession(std::move(socket));
				}
				if (_running)
				{
					_accept();
				}
			});
		}

		void _addSession(TCP::socket socket)
		{
			const ConnectionID id = _nextConnectionID++;
			auto session = std::make_shared<Session>(id, std::move(socket));
			_sessions.emplace(id, session);
			_owner._notifyConnection(id);
			_readHeader(std::move(session));
		}

		void _readHeader(const std::shared_ptr<Session> &session)
		{
			asio::async_read(session->socket, asio::buffer(session->header),
				[this, session](const std::error_code &error, std::size_t) {
					if (error)
					{
						_disconnect(session->id);
						return;
					}
					_handleHeader(session);
				});
		}

		void _handleHeader(const std::shared_ptr<Session> &session)
		{
			try
			{
				const auto header = NetworkInternal::decode(session->header);
				_readPayload(session, header);
			}
			catch (...)
			{
				_disconnect(session->id);
			}
		}

		void _readPayload(const std::shared_ptr<Session> &session, NetworkInternal::FrameHeader header)
		{
			session->payload.resize(header.payloadSize);
			if (session->payload.empty())
			{
				_publish(session, Message(header.type));
				return;
			}
			asio::async_read(session->socket, asio::buffer(session->payload),
				[this, session, header](const std::error_code &error, std::size_t) {
					if (error)
					{
						_disconnect(session->id);
						return;
					}
					_publish(session, Message(header.type, std::move(session->payload)));
				});
		}

		void _publish(const std::shared_ptr<Session> &session, Message message)
		{
			_owner._publish(ReceivedMessage{session->id, std::move(message)});
			_readHeader(session);
		}

		void _queueWrite(const std::shared_ptr<Session> &session, Frame frame)
		{
			const bool idle = session->writes.empty();
			session->writes.push_back(std::move(frame));
			if (idle)
			{
				_writeNext(session);
			}
		}

		void _writeNext(const std::shared_ptr<Session> &session)
		{
			if (session->writes.empty())
			{
				return;
			}
			asio::async_write(session->socket, asio::buffer(*session->writes.front()),
				[this, session](const std::error_code &error, std::size_t) {
					if (error)
					{
						_disconnect(session->id);
						return;
					}
					session->writes.pop_front();
					_writeNext(session);
				});
		}

		void _closeSession(const std::shared_ptr<Session> &session) noexcept
		{
			std::error_code ignored;
			session->socket.cancel(ignored);
			session->socket.shutdown(TCP::socket::shutdown_both, ignored);
			session->socket.close(ignored);
			session->writes.clear();
		}

		void _disconnect(ConnectionID id)
		{
			const auto iterator = _sessions.find(id);
			if (iterator == _sessions.end())
			{
				return;
			}
			_closeSession(iterator->second);
			_sessions.erase(iterator);
			_owner._notifyDisconnection(id);
		}

		void _shutdown()
		{
			if (!_running.exchange(false))
			{
				return;
			}

			std::error_code ignored;
			if (_acceptor != nullptr)
			{
				_acceptor->cancel(ignored);
				_acceptor->close(ignored);
			}

			std::vector<ConnectionID> connections;
			connections.reserve(_sessions.size());
			for (const auto &[id, session] : _sessions)
			{
				_closeSession(session);
				connections.push_back(id);
			}
			_sessions.clear();
			for (const ConnectionID id : connections)
			{
				_owner._notifyDisconnection(id);
			}
			_work.reset();
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
		explicit Impl(Server &owner) :
			_owner(owner)
		{
		}

		~Impl()
		{
			try
			{
				stop();
			}
			catch (...)
			{
			}
		}

		[[nodiscard]] bool isRunning() const noexcept
		{
			return _running.load();
		}

		[[nodiscard]] std::uint16_t port() const noexcept
		{
			return _port.load();
		}

		void start(std::uint16_t requestedPort)
		{
			stop();
			_context.restart();
			_work.emplace(_context.get_executor());
			_acceptor = std::make_unique<TCP::acceptor>(_context);

			try
			{
				_open(requestedPort);
			}
			catch (...)
			{
				_work.reset();
				_acceptor.reset();
				throw Exception("Unable to start the network server.", std::current_exception());
			}

			_running = true;
			_accept();
			_worker = std::jthread([this] {
				_context.run();
			});
		}

		void stop()
		{
			if (_running)
			{
				_invoke([this] {
					_shutdown();
				});
			}
			_joinWorker();
			_work.reset();
			_context.stop();
			_acceptor.reset();
			_port = 0;
		}

		void sendTo(ConnectionID connection, const Message &message)
		{
			_ensureRunning();
			Frame frame = NetworkInternal::encode(message);
			asio::post(_context, [this, connection, frame = std::move(frame)] {
				const auto iterator = _sessions.find(connection);
				if (iterator != _sessions.end())
				{
					_queueWrite(iterator->second, std::move(frame));
				}
			});
		}

		void sendToAll(const Message &message)
		{
			_ensureRunning();
			Frame frame = NetworkInternal::encode(message);
			asio::post(_context, [this, frame = std::move(frame)] {
				for (const auto &[id, session] : _sessions)
				{
					_queueWrite(session, frame);
				}
			});
		}

	private:
		void _open(std::uint16_t requestedPort)
		{
			_acceptor->open(TCP::v4());
			_acceptor->set_option(TCP::acceptor::reuse_address(true));
			_acceptor->bind(TCP::endpoint(TCP::v4(), requestedPort));
			_acceptor->listen();
			_port = _acceptor->local_endpoint().port();
		}

		void _ensureRunning() const
		{
			if (!_running)
			{
				throw Exception("Unable to send through a stopped network server.");
			}
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
