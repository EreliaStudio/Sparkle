#include "network/trait/object_request_handler_trait.hpp"
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
namespace spk::Network
{
	ObjectRequestHandlerTrait::ObjectRequestHandlerTrait(std::size_t maximumPending) :
		_maximumPending(maximumPending)
	{
		if (maximumPending == 0)
		{
			throw spk::Exception("Invalid request capacity");
		}
	}
	void ObjectRequestHandlerTrait::_requireRequestHandlerIdle() const
	{
	}
	bool ObjectRequestHandlerTrait::receiveRequest(PeerID peer, const Request &request)
	{
		_requireRequestHandlerIdle();
		OperationGuard guard(_requesting);
		if (request.session.isNull() || request.object.isNull() || request.id == 0)
		{
			throw spk::Exception("Invalid acquisition request");
		}
		if (_requestSession(peer) != request.session)
		{
			return false;
		}
		auto &history = _requests[peer];
		if (!_isNewRequest(peer, request.id))
		{
			return false;
		}
		if (!history.pending.contains(request.object) && history.pending.size() >= _maximumPending)
		{
			throw spk::Exception("Pending request limit reached");
		}
		history.pending.insert_or_assign(request.object, request);
		_recordRequest(peer, request.id);
		_requestObject(peer, request);
		return true;
	}
	bool ObjectRequestHandlerTrait::_isCurrentRequest(PeerID peer, const Request &request) const
	{
		if (_requestSession(peer) != request.session)
		{
			return false;
		}
		auto history = _requests.find(peer);
		if (history == _requests.end())
		{
			return false;
		}
		auto found = history->second.pending.find(request.object);
		return found != history->second.pending.end() && found->second == request;
	}
	void ObjectRequestHandlerTrait::_completeRequest(PeerID peer, ObjectID object)
	{
		auto found = _requests.find(peer);
		if (found != _requests.end())
		{
			found->second.pending.erase(object);
		}
	}
	void ObjectRequestHandlerTrait::_cancelPeerRequests(PeerID peer)
	{
		_requests.erase(peer);
		_clearRequestHistory(peer);
	}
	void ObjectRequestHandlerTrait::_cancelObjectRequests(ObjectID object)
	{
		for (auto &[peer, history] : _requests)
		{
			history.pending.erase(object);
		}
	}
}
