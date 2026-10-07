#pragma once
#include <optional>

#include "edit.hpp"
#include "network/message.hpp"
#include "types.hpp"
#include <cstdint>

namespace spk::Network
{
	// Immutable serialized application payload; absence denotes a removal, not an empty state.
	struct Update
	{
		SessionID session;
		ObjectID object;
		std::uint64_t tracking = 0, revision = 0;
		Edit edit = Edit::Set;
		std::optional<spk::Message> payload;
	};
} // namespace spk::Network
