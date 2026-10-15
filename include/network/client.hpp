#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "network/connection.hpp"
#include "network/message.hpp"
#include "network/message_dispatcher.hpp"
#include "network/page_failure_limits.hpp"
#include "network/transfer.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>

namespace spk
{
	class Client
	{
	public:
		using MessageDispatcher = spk::MessageDispatcher<Message>;
		using MessageQueue = ThreadSafeFIFO<Message>;
		using StatusProvider = ContractProvider<Connection, Connection::Status, Connection::Error>;
		using StatusCallback = StatusProvider::callback_type;
		using StatusContract = StatusProvider::Contract;

		enum class ConnectBehaviour
		{
			Throw,
			DisconnectPrevious
		};

		enum class HandshakeTimeoutBehaviour
		{
			FutureAttemptsOnly
		};

		enum class IncomingBudgetBehaviour
		{
			AccountPayloadAndMetadataPreservingAccepted
		};

		enum class PageSizeBehaviour
		{
			RespectNegotiatedLimit
		};

		enum class SendBehaviour
		{
			SynchronousAdmission
		};

		enum class MessageDispatchBehaviour
		{
			CallingThread
		};

	private:
		class Impl;
		std::unique_ptr<Impl> _impl;
		MessageQueue _messages;
		MessageDispatcher _messageDispatcher;
		StatusProvider _statusProvider;
		mutable std::mutex _connectionMutex;
		Connection _connection;
		bool _acceptIncoming = false;

		void _publish(Connection session, Message message);
		void _retireIncoming(Connection session);
		void _acceptConnection(Connection connection, Connection::ID id);
		[[nodiscard]] Connection _beginConnection();
		void _announcePending(Connection connection);
		void _setStatus(Connection connection, Connection::Status status, Connection::Error error = Connection::Error::None);

	public:
		Client();
		~Client();

		Client(const Client &) = delete;
		Client(Client &&) = delete;
		Client &operator=(const Client &) = delete;
		Client &operator=(Client &&) = delete;

		[[nodiscard]] StatusContract subscribeToConnectionStatusChange(StatusCallback callback);
		[[nodiscard]] Connection connection() const;
		[[nodiscard]] bool isConnected() const noexcept;
		Connection connect(std::string_view address, std::uint16_t port, ConnectBehaviour behaviour = ConnectBehaviour::Throw);
		void setHandshakeTimeout(std::chrono::milliseconds timeout, HandshakeTimeoutBehaviour behaviour = HandshakeTimeoutBehaviour::FutureAttemptsOnly);
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		void setMaxMessageBytes(std::size_t bytes);
		void setMaxIncomingBytes(std::size_t bytes, IncomingBudgetBehaviour behaviour = IncomingBudgetBehaviour::AccountPayloadAndMetadataPreservingAccepted);
		void setMaxPageBytes(std::size_t bytes, PageSizeBehaviour behaviour = PageSizeBehaviour::RespectNegotiatedLimit);
		void disconnect();
		void setPageFailureLimits(PageFailureLimits limits);
		Transfer send(const Message &message, Message::Priority priority = Message::Priority::Normal, SendBehaviour behaviour = SendBehaviour::SynchronousAdmission);
		[[nodiscard]] MessageQueue &messages() noexcept;
		[[nodiscard]] MessageDispatcher &messageDispatcher() noexcept;
		void treatMessages(MessageDispatchBehaviour behaviour = MessageDispatchBehaviour::CallingThread);
	};
}
