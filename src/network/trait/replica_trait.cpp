#include "network/trait/replica_trait.hpp"

namespace spk::Network
{
	void ReplicaTrait::_clearReplicas()
	{
		for (const auto id : _activeReplicas())
		{
			_removeReplica(id);
		}
		_clearHistory();
		_onReplicasCleared();
	}
	SessionID ReplicaTrait::_replicaSession() const
	{
		return _session;
	}
	bool &ReplicaTrait::_replicaOperation()
	{
		return _active;
	}
	void ReplicaTrait::_requireReplicaIdle() const
	{
		if (_active)
		{
			throw spk::Exception("Replica mutation during application hook");
		}
	}
	void ReplicaTrait::_onReplicasCleared()
	{
	}
	void ReplicaTrait::_onReplicaRemoved(ObjectID)
	{
	}
	bool ReplicaTrait::_acceptUpdate(const Update<spk::Message> &update)
	{
		if (_session.isNull() || update.session != _session)
		{
			return false;
		}
		if ((update.edit == Edit::Set) != static_cast<bool>(update.payload))
		{
			throw spk::Exception("Invalid replica update state");
		}
		return _applyTracked(update.object, update.tracking, update.revision, update.edit, [&] {
			_applyReplica(update);
			if (update.edit != Edit::Set)
			{
				_onReplicaRemoved(update.object);
			}
		});
	}
	ReplicaTrait::ReplicaTrait(std::size_t maximumTracked) :
		ReplicaHistoryTrait(maximumTracked)
	{
	}
	bool ReplicaTrait::receiveUpdate(const Update<spk::Message> &update)
	{
		OperationGuard guard(_active);
		return _acceptUpdate(update);
	}
	void ReplicaTrait::resetSession(SessionID session)
	{
		OperationGuard guard(_active);
		if (session.isNull())
		{
			throw spk::Exception("Null network session");
		}
		if (session != _session)
		{
			_clearReplicas();
			_session = session;
		}
	}
	void ReplicaTrait::closeSession()
	{
		OperationGuard guard(_active);
		_clearReplicas();
		_session = {};
	}
}
