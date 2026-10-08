#pragma once
#include "network/replication/update.hpp"
#include "replicable_trait.hpp"
namespace spk::Network
{
	class ReplicaApplicationTrait
	{
	protected:
		[[nodiscard]] virtual ReplicableTrait *_findReplica(ObjectID id) = 0;
		// Creation must leave storage unchanged on failure. Removal must be idempotent
		// and must succeed when rolling back a failed initial state application.
		[[nodiscard]] virtual ReplicableTrait &_createReplica(ObjectID id) = 0;
		virtual void _removeReplica(ObjectID id) = 0;
		void _applyReplica(const Update<spk::Message> &update);

	public:
		virtual ~ReplicaApplicationTrait() = default;
	};
}
