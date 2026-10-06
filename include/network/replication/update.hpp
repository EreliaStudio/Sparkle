#pragma once

#include "edit.hpp"
#include "types.hpp"
#include <cstdint>
#include <memory>

namespace spk::Network
{
	// Immutable data snapshot, never a live application entity.
	template <typename State>
	struct Update
	{
		SessionID session;
		ObjectID object;
		std::uint64_t tracking = 0, revision = 0;
		Edit edit = Edit::Set;
		std::shared_ptr<const State> state;
	};
} // namespace spk::Network
