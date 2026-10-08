#pragma once
#include "network/replication/edit.hpp"
#include <cstdint>

namespace spk::Network
{
	// Determines whether a revision supersedes the currently tracked state.
	class ReplicaRevisionTrait
	{
	protected:
		[[nodiscard]] static bool _obsoleteRevision(std::uint64_t currentIdentity, std::uint64_t currentRevision, bool active,
			std::uint64_t identity, std::uint64_t revision, Edit edit) noexcept;

	public:
		virtual ~ReplicaRevisionTrait() = default;
	};
}
