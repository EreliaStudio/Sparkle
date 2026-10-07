#pragma once
#include "network/replication/update.hpp"
#include "replicable_trait.hpp"
namespace spk::Network
{
	template <typename State>
	class ReplicaApplicationTrait
	{
	protected:
		[[nodiscard]] virtual ReplicableTrait<State> *_findReplica(ObjectID id) = 0;
		// Creation must leave storage unchanged on failure. Removal must be idempotent
		// and must succeed when rolling back a failed initial state application.
		[[nodiscard]] virtual ReplicableTrait<State> &_createReplica(ObjectID id) = 0;
		virtual void _removeReplica(ObjectID id) = 0;
		void _applyReplica(const Update<State> &update)
		{
			if (update.edit != Edit::Set)
			{
				_removeReplica(update.object);
				return;
			}
			if (auto *replica = _findReplica(update.object))
			{
				replica->applyNetworkState(*update.state);
				return;
			}
			auto &created = _createReplica(update.object);
			try
			{
				created.applyNetworkState(*update.state);
			} catch (...)
			{
				_removeReplica(update.object);
				throw;
			}
		}

	public:
		virtual ~ReplicaApplicationTrait() = default;
	};
}
