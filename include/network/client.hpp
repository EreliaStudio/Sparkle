#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace spk
{
	class Client
	{
	public:
		using MessageQueue = ThreadSafeFIFO<Message>;
		using ConnectionProvider = ContractProvider<>;
		using ConnectionCallback = ConnectionProvider::callback_type;
		using ConnectionContract = ConnectionProvider::Contract;
		using DisconnectionProvider = ContractProvider<>;
		using DisconnectionCallback = DisconnectionProvider::callback_type;
		using DisconnectionContract = DisconnectionProvider::Contract;

	private:
		class Impl;
		std::unique_ptr<Impl> _impl;
		MessageQueue _messages;
		ConnectionProvider _connectionProvider;
		DisconnectionProvider _disconnectionProvider;

		void _publish(Message message);
		void _notifyConnection();
		void _notifyDisconnection();

	public:
		Client();
		~Client();

		Client(const Client &) = delete;
		Client(Client &&) = delete;
		Client &operator=(const Client &) = delete;
		Client &operator=(Client &&) = delete;

		[[nodiscard]] ConnectionContract subscribeToConnection(ConnectionCallback callback);
		[[nodiscard]] DisconnectionContract subscribeToDisconnection(DisconnectionCallback callback);

		[[nodiscard]] bool isConnected() const noexcept;
		void connect(std::string_view address, std::uint16_t port);
		void disconnect();
		void send(const Message &message);
		[[nodiscard]] MessageQueue &messages() noexcept;
	};
}
