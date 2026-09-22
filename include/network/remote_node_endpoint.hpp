#pragma once

#include "container/thread_safe_fifo.hpp"
#include "network/server.hpp"

#include <cstdint>
#include <vector>

namespace spk
{
	struct RemoteRequest
	{
		ConnectionID proxyConnection = InvalidConnectionID;
		ConnectionID originConnection = InvalidConnectionID;
		Message message;
	};

	class RemoteNodeEndpoint
	{
	public:
		using RequestQueue = ThreadSafeFIFO<RemoteRequest>;

	private:
		Server _server;
		RequestQueue _requests;
		std::vector<ReceivedMessage> _received;

	public:
		void start(std::uint16_t port);
		void stop();
		[[nodiscard]] std::uint16_t port() const noexcept;
		[[nodiscard]] bool isRunning() const noexcept;

		void dispatch();
		[[nodiscard]] RequestQueue &requests() noexcept;
		void reply(const RemoteRequest &request, Message message);
	};
}
