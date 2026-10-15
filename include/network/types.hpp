#pragma once

#include "network/connection.hpp"
#include "network/message.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace spk
{
	using ConnectionID = Connection::ID;
	inline constexpr ConnectionID InvalidConnectionID = Connection::InvalidID;

	struct ReceivedMessage
	{
		ConnectionID emitter = InvalidConnectionID;
		Message message;
	};

	struct OutgoingMessage
	{
		std::optional<ConnectionID> recipient;
		Message message;

		[[nodiscard]] static OutgoingMessage to(ConnectionID connection, Message message)
		{
			return OutgoingMessage{connection, std::move(message)};
		}

		[[nodiscard]] static OutgoingMessage broadcast(Message message)
		{
			return OutgoingMessage{std::nullopt, std::move(message)};
		}
	};
}
