#pragma once

#include "network/internal/server_handshake.hpp"
#include "network/internal/server_send_batch.hpp"
#include "network/internal/socket.hpp"
#include "network/internal/transfer_reception.hpp"
#include "network/internal/transfer_sender.hpp"
#include "network/internal/transmission_frame.hpp"
#include "network/page_failure_limits.hpp"
#include "network/types.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

namespace spk::NetworkInternal
{
	struct ServerCallbacks
	{
		std::function<void(ReceivedMessage)> onMessage;
		std::function<void(ConnectionID)> onConnection;
		std::function<std::vector<Connection>()> snapshotConnections;
		std::function<void(ConnectionID)> onReady;
		std::function<void(ConnectionID, Connection::Error)> onDisconnection;
	};

	// The worker owns transport state; platform backends implement only kernel I/O.
	class ServerBackend
	{
	protected:
		struct Session
		{
			ConnectionID id;
			Socket socket;
			std::vector<std::byte> receivedBytes;
			TransferReception reception;
			ServerHandshake handshake;
			TransferSender transfers;
			std::deque<TransmissionFrame> outgoing;
			std::size_t outgoingOffset = 0;
			bool writeInFlight = false;
			std::size_t queuedBytes = 0;
			std::size_t reservedOutgoingBytes = 0;
			std::unordered_map<spk::Transfer::ID, OutgoingReservation> outgoingReservations;

			Session(ConnectionID id, Socket socket, PageFailureLimits limits, std::chrono::milliseconds timeout, std::shared_ptr<ReassemblyBudget> sharedBudget, std::uint32_t maximumPageBytes);
		};
		enum class CommandKind
		{
			Send,
			ConfigureLimits,
			ConfigureHandshakeTimeout,
			ConfigureIncomingTransferTimeout,
			ConfigureAcknowledgmentTimeout,
			ConfigureIncomingLimits,
			Stop
		};

		struct Command
		{
			CommandKind kind;
			PageFailureLimits limits;
			std::chrono::milliseconds handshakeTimeout = Handshake::DefaultTimeout;
			std::chrono::milliseconds transferTimeout = std::chrono::seconds(60);
			std::shared_ptr<ServerSendBatch> batch;
			std::shared_ptr<CommandBudget::Lease> reservation;
			std::size_t maxMessageBytes = 128u * 1024u * 1024u;
			std::size_t maxIncomingBytes = 512u * 1024u * 1024u;
		};

	private:
		ServerCallbacks _callbacks;
		CommandBudget _commandBudget{256u * 1024u * 1024u};
		std::mutex _commandMutex;
		std::deque<Command> _commands;
		PageFailureLimits _failureLimits;
		std::chrono::milliseconds _handshakeTimeout = Handshake::DefaultTimeout;
		std::chrono::milliseconds _incomingTransferTimeout = std::chrono::seconds(60);
		std::chrono::milliseconds _acknowledgmentTimeout = std::chrono::seconds(30);
		std::atomic_size_t _maxMessageBytes = 128u * 1024u * 1024u;
		std::atomic_size_t _maxIncomingBytes = 512u * 1024u * 1024u;
		std::atomic_size_t _maxPageBytes = DefaultMaximumPageBytes;
		std::shared_ptr<ReassemblyBudget> _incomingBudget = std::make_shared<ReassemblyBudget>();

		void _notifyReady(ConnectionID id) noexcept;
		void _notifyDisconnection(ConnectionID id, Connection::Error error) noexcept;
		void _publish(ReceivedMessage message) noexcept;
		void _queueHandshake(Session &session, Handshake::Kind kind, Connection::Error error = Connection::Error::None);
		void _handleHello(Session &session, std::span<const std::byte> bytes);
		void _handshakeWritten(Session &session, std::uint16_t kind);
		void _queueResponse(Session &session, const TransferResponse &response);
		[[nodiscard]] bool _extractMessage(Session &session, std::size_t &consumed);
		void _releaseReservation(Session &session, spk::Transfer::ID id) noexcept;
		void _queueMessage(const std::shared_ptr<Session> &session, const Message &message, Message::Priority priority, ServerSendBatch::Recipient &recipient);
		[[nodiscard]] std::deque<Command> _takeCommands();
		void _processCommand(Command command);
		void _sendBatch(ServerSendBatch &batch);
		void _configureIncomingLimits(std::size_t messageBytes, std::size_t incomingBytes);

	protected:
		Socket _listener;
		std::jthread _worker;
		std::atomic_bool _running = false;
		std::atomic_uint16_t _port = 0;
		std::unordered_map<ConnectionID, std::shared_ptr<Session>> _sessions;
		bool _stopping = false;

		virtual void _openEvents() = 0;
		virtual void _listen() = 0;
		virtual void _closeEvents() noexcept = 0;
		virtual void _wake() noexcept = 0;
		virtual void _run() noexcept = 0;
		virtual void _writeSession(const std::shared_ptr<Session> &session) = 0;
		virtual void _requestWrite(Session &session) = 0;
		virtual void _unregisterSocket(const Socket &socket) noexcept;

		[[nodiscard]] std::shared_ptr<Session> _makeSession(Socket socket);
		void _notifyConnection(ConnectionID id) noexcept;
		void _appendReceived(Session &session, const std::byte *data, std::size_t size);
		[[nodiscard]] bool _prepareWrite(Session &session);
		void _completeFrame(Session &session, std::size_t bytes);
		void _disconnect(ConnectionID id) noexcept;
		void _disconnectAll() noexcept;
		void _enqueue(Command command);
		void _processCommands();
		void _beginStop() noexcept;
		void _expireReassemblies();

		explicit ServerBackend(ServerCallbacks callbacks);

	public:
		virtual ~ServerBackend() = default;
		[[nodiscard]] bool isRunning() const noexcept;
		[[nodiscard]] std::uint16_t port() const noexcept;
		void start(std::uint16_t port);
		void stop();
		void setPageFailureLimits(PageFailureLimits limits);
		void setHandshakeTimeout(std::chrono::milliseconds timeout);
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		void setGlobalMaxQueuedBytes(std::size_t bytes);
		void setMaxMessageBytes(std::size_t bytes);
		void setMaxIncomingBytes(std::size_t bytes);
		void setMaxPageBytes(std::size_t bytes);
		void setGlobalMaxIncomingBytes(std::size_t bytes);
		void send(std::shared_ptr<ServerSendBatch> batch);
	};

	[[nodiscard]] std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks);
}
