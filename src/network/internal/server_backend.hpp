#pragma once

#include "network/internal/server_send_batch.hpp"
#include "network/page_failure_limits.hpp"
#include "network/types.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
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

	class ServerBackend
	{
	public:
		virtual ~ServerBackend() = default;

		[[nodiscard]] virtual bool isRunning() const noexcept = 0;
		[[nodiscard]] virtual std::uint16_t port() const noexcept = 0;
		virtual void start(std::uint16_t port) = 0;
		virtual void stop() = 0;
		virtual void setPageFailureLimits(PageFailureLimits limits) = 0;
		virtual void setHandshakeTimeout(std::chrono::milliseconds timeout) = 0;
		virtual void setIncomingTransferTimeout(std::chrono::milliseconds timeout) = 0;
		virtual void setAcknowledgmentTimeout(std::chrono::milliseconds timeout) = 0;
		virtual void setGlobalMaxQueuedBytes(std::size_t bytes) = 0;
		virtual void setMaxMessageBytes(std::size_t bytes) = 0;
		virtual void setMaxIncomingBytes(std::size_t bytes) = 0;
		virtual void setMaxPageBytes(std::size_t bytes) = 0;
		virtual void setGlobalMaxIncomingBytes(std::size_t bytes) = 0;
		virtual void send(std::shared_ptr<ServerSendBatch> batch) = 0;
	};

	[[nodiscard]] std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks);
}
