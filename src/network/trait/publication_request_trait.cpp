#include "network/trait/publication_request_trait.hpp"
namespace spk::Network
{
	PublicationRequestTrait::PublicationRequestTrait() :
		PublicationRequestTrait(Configuration{})
	{
	}
	PublicationRequestTrait::PublicationRequestTrait(Configuration configuration) :
		PublicationTrait(configuration),
		ObjectRequestHandlerTrait(configuration.maximumObjects)
	{
	}
	void PublicationRequestTrait::_requireRequestHandlerIdle() const
	{
		_requirePublicationIdle();
	}
	std::optional<SessionID> PublicationRequestTrait::_requestSession(PeerID peer) const
	{
		return _findSession(peer);
	}
	void PublicationRequestTrait::_onPeerClosed(PeerID peer)
	{
		_cancelPeerRequests(peer);
	}
	void PublicationRequestTrait::_onObjectForgotten(PeerID peer, ObjectID object)
	{
		_completeRequest(peer, object);
	}
	void PublicationRequestTrait::_onObjectDestroyed(ObjectID object)
	{
		_cancelObjectRequests(object);
	}
	void PublicationRequestTrait::_requestObject(PeerID peer, const Request &request)
	{
		if (_hasSnapshot(request.object))
		{
			(void)acceptRequest(peer, request);
		}
		else
		{
			(void)rejectRequest(peer, request);
		}
	}
	bool PublicationRequestTrait::acceptRequest(PeerID peer, const Request &request)
	{
		OperationGuard guard(_publicationOperation());
		if (!_isCurrentRequest(peer, request))
		{
			return false;
		}
		_follow(peer, request.object, request.id);
		_completeRequest(peer, request.object);
		return true;
	}
	bool PublicationRequestTrait::fulfillRequest(PeerID peer, const Request &request, spk::Message payload)
	{
		OperationGuard guard(_publicationOperation());
		if (!_isCurrentRequest(peer, request))
		{
			return false;
		}
		_room(peer, request.object);
		_publish(request.object, std::move(payload));
		_follow(peer, request.object, request.id);
		_completeRequest(peer, request.object);
		return true;
	}
	bool PublicationRequestTrait::rejectRequest(PeerID peer, const Request &request)
	{
		OperationGuard guard(_publicationOperation());
		if (!_isCurrentRequest(peer, request) || !_sendRejection(peer, request))
		{
			return false;
		}
		_completeRequest(peer, request.object);
		return true;
	}
}
