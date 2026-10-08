#include "network/trait/replica_revision_trait.hpp"

namespace spk::Network
{
	bool ReplicaRevisionTrait::_obsoleteRevision(std::uint64_t currentIdentity, std::uint64_t currentRevision, bool active,
		std::uint64_t identity, std::uint64_t revision, Edit edit) noexcept
	{
		if (identity != currentIdentity)
		{
			return identity < currentIdentity;
		}
		return !active || (edit == Edit::Set && revision <= currentRevision);
	}
}
