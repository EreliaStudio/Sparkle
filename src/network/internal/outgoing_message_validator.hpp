#pragma once

#include "exception.hpp"
#include "network/message.hpp"

#include <algorithm>
#include <cstddef>

namespace spk::NetworkInternal
{
	class OutgoingMessageValidator final
	{
	public:
		static void validate(const Message &message, Message::Priority priority)
		{
			if (static_cast<std::size_t>(priority) > static_cast<std::size_t>(Message::Priority::High))
			{
				throw Exception("Invalid network message priority.");
			}
			if (message.size() > 128u * 1024u * 1024u)
			{
				throw Exception("Outgoing logical message exceeds the network limit.");
			}
		}

		static void validateCapacity(const Message &message, std::size_t available)
		{
			if (std::max<std::size_t>(1, message.size()) > available)
			{
				throw Exception("Insufficient network transfer capacity.");
			}
		}
	};
}
