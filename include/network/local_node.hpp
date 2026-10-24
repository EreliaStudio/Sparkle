#pragma once

#include "network/node.hpp"

namespace spk
{
	class LocalNode final : public Node
	{
	public:
		using IncomingQueue = ThreadSafeFIFO<ReceivedMessage>;

	private:
		IncomingQueue _incoming;
		OutgoingQueue _outgoing;

	public:
		void receive(ReceivedMessage message) override;
		[[nodiscard]] OutgoingQueue &outgoing() override;
		[[nodiscard]] IncomingQueue &incoming() noexcept;

		void sendTo(ConnectionID connection, Message message);
		void reply(const ReceivedMessage &request, Message message);
		void broadcast(Message message);
	};
}
