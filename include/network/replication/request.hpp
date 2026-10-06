#pragma once

#include "types.hpp"
#include <compare>
#include <cstdint>

namespace spk::Network
{
	struct Request
	{
		SessionID session;
		ObjectID object;
		std::uint64_t attempt = 0;
		auto operator<=>(const Request &) const = default;
	};
} // namespace spk::Network
