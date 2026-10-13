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
		// Begins one asynchronous attempt; the returned handle is initially Pending.
		// A previous attempt must be explicitly disconnected before reconnecting.
		Connection connect(std::string_view address, std::uint16_t port);
		// Applies to attempts started after this call, not an active handshake.
		void setHandshakeTimeout(std::chrono::milliseconds timeout);
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		// Maximum logical payload for both incoming and outgoing messages.
		void setMaxMessageBytes(std::size_t bytes);
		// Incomplete reassembly budget: logical payload plus modeled transfer/page metadata.
		// This is not a process-wide RSS or allocator-capacity limit.
		// Counts incomplete logical payloads and modeled reassembly metadata, not allocator overhead.
		void setMaxIncomingBytes(std::size_t bytes);
		// Outgoing page payload is bounded by both the configured and negotiated limits.
		void setMaxPageBytes(std::size_t bytes);
		void disconnect();
		void setPageFailureLimits(PageFailureLimits limits);
		// Synchronously rejects invalid or unaffordable submissions; accepted transfers
		// report transport acknowledgment or failure asynchronously through the handle.
		// Admission is synchronous; the returned transfer reports the asynchronous delivery outcome.
		Transfer send(const Message &message, Message::Priority priority = Message::Priority::Normal);
		[[nodiscard]] MessageQueue &messages() noexcept;
		[[nodiscard]] MessageDispatcher &messageDispatcher() noexcept;
		// Dispatch a receive batch on the calling thread; do not also drain messages().
		void treatMessages();
	};
}
