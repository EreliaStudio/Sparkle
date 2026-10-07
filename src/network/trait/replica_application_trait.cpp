#include "network/trait/replica_application_trait.hpp"

namespace spk::Network
{
	void ReplicaApplicationTrait::_applyReplica(const Update &update)
	{
		if (update.edit != Edit::Set)
		{
			_removeReplica(update.object);
			return;
		}
		if (auto *replica = _findReplica(update.object))
		{
			replica->readNetworkState(update.payload->reader());
			return;
		}
		auto &created = _createReplica(update.object);
		try
		{
			created.readNetworkState(update.payload->reader());
		} catch (...)
		{
			_removeReplica(update.object);
			throw;
		}
	}
}
