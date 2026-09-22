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
	private:
		Client _client;
		OutgoingQueue _outgoing;
		std::vector<Message> _responses;

		void _collectResponses();

	public:
		[[nodiscard]] bool isConnected() const noexcept;
		void connect(std::string_view address, std::uint16_t port);
		void disconnect();

		void receive(ReceivedMessage message) override;
		[[nodiscard]] OutgoingQueue &outgoing() override;
	};
}
