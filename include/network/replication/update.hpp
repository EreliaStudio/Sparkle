#pragma once
#include <optional>

#include "edit.hpp"
#include "network/message.hpp"
#include "types.hpp"
#include <cstdint>

namespace spk::Network
{
	// Absence denotes a removal; State may be a typed value or an immutable captured Message.
	template <typename State = spk::Message>
	struct Update
	{
		SessionID session;
		ObjectID object;
		std::uint64_t tracking = 0, revision = 0;
		Edit edit = Edit::Set;
		std::optional<State> payload;
	};
} // namespace spk::Network
