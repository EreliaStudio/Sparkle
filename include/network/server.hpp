#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "network/types.hpp"

#include <cstdint>
#include <memory>
#include <span>

namespace spk
{
	class Server
	{
	public:
		using MessageQueue = ThreadSafeFIFO<ReceivedMessage>;
		using ConnectionProvider = ContractProvider<ConnectionID>;
		using ConnectionCallback = ConnectionProvider::callback_type;
		using ConnectionContract = ConnectionProvider::Contract;
		using DisconnectionProvider = ContractProvider<ConnectionID>;
		using DisconnectionCallback = DisconnectionProvider::callback_type;
		using DisconnectionContract = DisconnectionProvider::Contract;

	private:
		class Impl;
		std::unique_ptr<Impl> _impl;
		MessageQueue _messages;
		ConnectionProvider _connectionProvider;
		DisconnectionProvider _disconnectionProvider;

		void _publish(ReceivedMessage message);
		void _notifyConnection(ConnectionID connection);
		void _notifyDisconnection(ConnectionID connection);

	public:
		Server();
		~Server();

		Server(const Server &) = delete;
		Server(Server &&) = delete;
		Server &operator=(const Server &) = delete;
		Server &operator=(Server &&) = delete;

		[[nodiscard]] ConnectionContract subscribeToConnection(ConnectionCallback callback);
		[[nodiscard]] DisconnectionContract subscribeToDisconnection(DisconnectionCallback callback);

		[[nodiscard]] bool isRunning() const noexcept;
		[[nodiscard]] std::uint16_t port() const noexcept;
		void start(std::uint16_t port);
		void stop();
		void sendTo(ConnectionID connection, const Message &message);
		void sendTo(std::span<const ConnectionID> connections, const Message &message);
		void sendToAll(const Message &message);
		[[nodiscard]] MessageQueue &messages() noexcept;
	};
}
