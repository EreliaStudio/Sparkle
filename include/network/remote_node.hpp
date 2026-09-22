#pragma once

#include "network/client.hpp"
#include "network/node.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace spk
{
	class RemoteNode final : public Node
	{
	public:
		using IncomingQueue = ThreadSafeFIFO<ReceivedMessage>;

	private:
		Client _client;
		IncomingQueue _incoming;
		OutgoingQueue _outgoing;
		std::vector<ReceivedMessage> _requests;
		std::vector<Message> _responses;

		void _forwardRequests();
		void _collectResponses();

	public:
		[[nodiscard]] bool isConnected() const noexcept;
		void connect(std::string_view address, std::uint16_t port);
		void disconnect();
		void dispatch();

		void receive(ReceivedMessage message) override;
		[[nodiscard]] OutgoingQueue &outgoing() override;
	};
}
