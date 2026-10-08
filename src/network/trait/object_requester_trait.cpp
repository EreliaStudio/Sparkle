#include "network/trait/object_requester_trait.hpp"
#include "exception.hpp"
#include "network/replication/operation_guard.hpp"
namespace spk::Network
{
	ObjectRequesterTrait::ObjectRequesterTrait(std::size_t maximumRequests) :
		_maximumRequests(maximumRequests)
	{
		if (maximumRequests == 0)
		{
			throw spk::Exception("Invalid acquisition capacity");
		}
	}
	bool &ObjectRequesterTrait::_requestOperation()
	{
		return _activeRequest;
	}
	bool ObjectRequesterTrait::_sendRequest(const Request &request)
	{
		auto [entry, inserted] = _requests.try_emplace(request.object, Acquisition{request.id});
		try
		{
			if (_sendObjectRequest(request))
			{
				entry->second = {request.id};
				return true;
			}
		} catch (...)
		{
			if (inserted)
			{
				_requests.erase(entry);
			}
			throw;
		}
		if (inserted)
		{
			_requests.erase(entry);
		}
		return false;
	}
	bool ObjectRequesterTrait::requestObject(ObjectID object)
	{
		OperationGuard guard(_requestOperation());
		const auto session = _requestSession();
		if (session.isNull() || object.isNull() || (!_requests.contains(object) && _requests.size() >= _maximumRequests))
		{
			throw spk::Exception("Invalid request identity, session or capacity");
		}
		return _sendRequest({session, object, _requestIDs.next()});
	}
	void ObjectRequesterTrait::cancelRequest(ObjectID object)
	{
		OperationGuard guard(_requestOperation());
		_eraseAcquisition(object);
	}
	std::optional<ObjectRequesterTrait::RequestStatus> ObjectRequesterTrait::requestStatus(ObjectID object) const
	{
		auto found = _requests.find(object);
		return found == _requests.end() ? std::nullopt : std::optional{found->second.status};
	}
	bool ObjectRequesterTrait::_completeAcquisition(ObjectID object, spk::Message::RequestID requestID)
	{
		auto found = _requests.find(object);
		if (found == _requests.end() || found->second.id != requestID || found->second.status != RequestStatus::Pending)
		{
			return false;
		}
		found->second.status = RequestStatus::Ready;
		return true;
	}
	bool ObjectRequesterTrait::_rejectAcquisition(const Request &request)
	{
		auto found = _requests.find(request.object);
		if (request.session != _requestSession() || found == _requests.end() || found->second.id != request.id || found->second.status != RequestStatus::Pending)
		{
			return false;
		}
		found->second.status = RequestStatus::Failed;
		return true;
	}
	void ObjectRequesterTrait::_eraseAcquisition(ObjectID object)
	{
		_requests.erase(object);
	}
	void ObjectRequesterTrait::_clearAcquisitions()
	{
		_requests.clear();
	}
}
