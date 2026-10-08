#pragma once

#include "network/message.hpp"
#include "types.hpp"
#include <compare>

namespace spk::Network
{
	struct Request
	{
		SessionID session;
		ObjectID object;
		spk::Message::RequestID id = 0;
		auto operator<=>(const Request &) const = default;
	};
} // namespace spk::Network
