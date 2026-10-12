#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "network/message_dispatcher.hpp"
#include "network/page_failure_limits.hpp"
#include "network/types.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace spk
{
	class Server
	{
	public:
		using MessageDispatcher = spk::MessageDispatcher<ReceivedMessage>;
		using MessageQueue = ThreadSafeFIFO<ReceivedMessage>;
		using StatusProvider = ContractProvider<Connection, Connection::Status, Connection::Error>;
		using StatusCallback = StatusProvider::callback_type;
		using StatusContract = StatusProvider::Contract;

	private:
		class Impl;
		std::unique_ptr<Impl> _impl;
		MessageQueue _messages;
		MessageDispatcher _messageDispatcher;
		StatusProvider _statusProvider;
		mutable std::mutex _connectionsMutex;
		std::unordered_map<ConnectionID, Connection> _connections;

		void _publish(ReceivedMessage message);
		void _notifyConnection(ConnectionID connection);
		void _notifyReady(ConnectionID connection);
		void _notifyDisconnection(ConnectionID connection, Connection::Error error);

	public:
		Server();
		~Server();

		Server(const Server &) = delete;
		Server(Server &&) = delete;
		Server &operator=(const Server &) = delete;
		Server &operator=(Server &&) = delete;

		[[nodiscard]] StatusContract subscribeToConnectionStatusChange(StatusCallback callback);
		[[nodiscard]] Connection connection(ConnectionID id) const;
		[[nodiscard]] bool isRunning() const noexcept;
		[[nodiscard]] std::uint16_t port() const noexcept;
		void start(std::uint16_t port);
		void stop();
		void setPageFailureLimits(PageFailureLimits limits);
		void setHandshakeTimeout(std::chrono::milliseconds timeout);
		void sendTo(const Connection &connection, const Message &message, Message::Priority priority = Message::Priority::Normal);
		void sendTo(const ConnectionSet &connections, const Message &message, Message::Priority priority = Message::Priority::Normal);
		void sendToAll(const Message &message, Message::Priority priority = Message::Priority::Normal);
		[[nodiscard]] MessageQueue &messages() noexcept;
		[[nodiscard]] MessageDispatcher &messageDispatcher() noexcept;
		// Dispatch a receive batch on the calling thread; do not also drain messages().
		void treatMessages();
	};
}
