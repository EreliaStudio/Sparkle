#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "network/message_dispatcher.hpp"
#include "network/page_failure_limits.hpp"
#include "network/transfer.hpp"
#include "network/types.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace spk
{
	class Server
	{
	public:
		using MessageDispatcher = spk::MessageDispatcher<ReceivedMessage>;
		using MessageQueue = ThreadSafeFIFO<ReceivedMessage>;
		using TransferResults = std::unordered_map<ConnectionID, Transfer>;
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
		std::atomic_size_t _maxMessageBytes = 128u * 1024u * 1024u;

		void _publish(ReceivedMessage message);
		[[nodiscard]] std::vector<Connection> _snapshotConnections() const;
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
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		// Shared outgoing ceiling across commands, logical transfers, encoded frames and in-flight writes.
		// This is reserved transport accounting, not a bound on allocator or OS socket overhead.
		// Lowering the cap blocks future reservations without evicting accepted work.
		void setGlobalMaxQueuedBytes(std::size_t bytes);
		void setMaxMessageBytes(std::size_t bytes);
		// Per-session incomplete payloads and modeled reassembly metadata; excludes allocator overhead.
		// Reductions constrain new transfers; accepted transfers retain their continuation allowance.
		void setMaxIncomingBytes(std::size_t bytes);
		void setMaxPageBytes(std::size_t bytes);
		// Shared incoming reassembly admission cap across all sessions (not total process memory).
		// Neither incoming limit accounts for allocator capacity or socket buffers.
		// Reductions preserve accepted-transfer allowances; increases also extend them.
		void setGlobalMaxIncomingBytes(std::size_t bytes);
		// Targeted send throws on invalid/Pending/disconnected handles or no capacity.
		Transfer sendTo(const Connection &connection, const Message &message, Message::Priority priority = Message::Priority::Normal);
		// Synchronous admission: Reject is all-or-nothing and throws before return.
		// Explicit targets must be valid and Connected, regardless of policy.
		// SkipRecipient reports capacity-limited explicit targets as Rejected transfers.
		// Returned transfer success means transport acknowledgment, not app receipt.
		TransferResults sendTo(const ConnectionSet &connections, const Message &message, Message::Priority priority = Message::Priority::Normal, Connection::OverflowPolicy policy = Connection::OverflowPolicy::Reject);
		// No clients returns an empty map immediately for every policy, with no traffic queued.
		// Only SkipRecipient excludes Pending peers, also returning empty if none remain.
		TransferResults sendToAll(const Message &message, Message::Priority priority = Message::Priority::Normal, Connection::OverflowPolicy policy = Connection::OverflowPolicy::Reject);
		[[nodiscard]] MessageQueue &messages() noexcept;
		[[nodiscard]] MessageDispatcher &messageDispatcher() noexcept;
		// Dispatch a receive batch on the calling thread; do not also drain messages().
		void treatMessages();
	};
}
