#pragma once

#include <chrono>
#include <type/uuid.hpp>

namespace spk::Network
{
	using ObjectID = spk::UUID;
	using PeerID = spk::UUID;
	using SessionID = spk::UUID;
	using Clock = std::chrono::steady_clock;
} // namespace spk::Network
