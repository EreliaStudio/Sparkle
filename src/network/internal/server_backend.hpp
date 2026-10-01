#pragma once

#include "network/types.hpp"

#include <cstdint>
#include <functional>
#include <memory>

namespace spk::NetworkInternal
{
	struct ServerCallbacks
	{
		std::function<void(ReceivedMessage)> onMessage;
		std::function<void(ConnectionID)> onConnection;
		std::function<void(ConnectionID)> onDisconnection;
	};

	class ServerBackend
	{
	public:
		virtual ~ServerBackend() = default;

		[[nodiscard]] virtual bool isRunning() const noexcept = 0;
		[[nodiscard]] virtual std::uint16_t port() const noexcept = 0;
		virtual void start(std::uint16_t port) = 0;
		virtual void stop() = 0;
		virtual void sendTo(ConnectionID connection, const Message &message) = 0;
		virtual void sendToAll(const Message &message) = 0;
	};

	[[nodiscard]] std::unique_ptr<ServerBackend> createServerBackend(ServerCallbacks callbacks);
}
