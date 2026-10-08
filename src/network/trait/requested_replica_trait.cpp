#include "network/trait/requested_replica_trait.hpp"
namespace spk::Network
{
	RequestedReplicaTrait::RequestedReplicaTrait(std::size_t maximumTracked) :
		ReplicaTrait(maximumTracked),
		ObjectRequesterTrait(maximumTracked)
	{
	}
	bool &RequestedReplicaTrait::_requestOperation()
	{
		return _replicaOperation();
	}
	SessionID RequestedReplicaTrait::_requestSession() const
	{
		return _replicaSession();
	}
	void RequestedReplicaTrait::_onReplicasCleared()
	{
		_clearAcquisitions();
	}
	void RequestedReplicaTrait::_onReplicaRemoved(ObjectID id)
	{
		_eraseAcquisition(id);
	}
	bool RequestedReplicaTrait::_receiveRequestedUpdate(const Update<spk::Message> &update, spk::Message::RequestID requestID)
	{
		if (update.session != _replicaSession() || update.session.isNull())
		{
			return false;
		}
		const bool accepted = _acceptUpdate(update);
		if (update.edit == Edit::Set && _tracksActive(update.object, update.tracking) &&
			_completeAcquisition(update.object, requestID))
		{
			return true;
		}
		return accepted;
	}
	bool RequestedReplicaTrait::receiveUpdate(const Update<spk::Message> &update, spk::Message::RequestID requestID)
	{
		OperationGuard guard(_replicaOperation());
		return _receiveRequestedUpdate(update, requestID);
	}
	bool RequestedReplicaTrait::receiveRejection(const Request &request)
	{
		OperationGuard guard(_replicaOperation());
		return _rejectAcquisition(request);
	}
}
