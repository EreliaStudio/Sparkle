#pragma once

#include "container/thread_safe_fifo.hpp"
#include "network/types.hpp"

namespace spk
{
	class Node
	{
	public:
		using OutgoingQueue = ThreadSafeFIFO<OutgoingMessage>;

		virtual ~Node() = default;

		virtual void receive(ReceivedMessage message) = 0;
		[[nodiscard]] virtual OutgoingQueue &outgoing() noexcept = 0;
	};
}
