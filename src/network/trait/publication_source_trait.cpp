#include "network/trait/publication_source_trait.hpp"

namespace spk::Network
{
	void PublicationSourceTrait::_requireServerIdle() const
	{
		_requirePublicationIdle();
	}
	void PublicationSourceTrait::_requireRequestHandlerIdle() const
	{
		_requirePublicationIdle();
	}
	std::optional<SessionID> PublicationSourceTrait::_requestSession(PeerID peer) const
	{
		return _findSession(peer);
	}
	void PublicationSourceTrait::_onPeerClosed(PeerID peer)
	{
		_cancelPeerRequests(peer);
	}
	void PublicationSourceTrait::_onObjectForgotten(PeerID peer, ObjectID object)
	{
		_completeRequest(peer, object);
	}
	void PublicationSourceTrait::_onObjectDestroyed(ObjectID object)
	{
		_cancelObjectRequests(object);
	}
	bool PublicationSourceTrait::_sendUpdate(PeerID peer, const Update<spk::Message> &update, spk::Message::RequestID requestID)
	{
		return _sendMessage(peer, _protocol.encode(update, requestID));
	}
	SessionID PublicationSourceTrait::_openHandshakePeer(PeerID peer)
	{
		return openPeer(peer);
	}
	void PublicationSourceTrait::_closeHandshakePeer(PeerID peer)
	{
		closePeer(peer);
	}
	std::optional<SessionID> PublicationSourceTrait::_findHandshakeSession(PeerID peer) const
	{
		return _findSession(peer);
	}
	void PublicationSourceTrait::_sendHandshakeSession(spk::ConnectionID connection, SessionID token, SessionID session)
	{
		(void)_sendTo(connection, _protocol.encodeHandshake(token, session));
	}
	void PublicationSourceTrait::_onServerConnectionClosed(spk::ConnectionID connection)
	{
		_closeHandshakeConnection(connection);
	}
	void PublicationSourceTrait::_onServerUnbinding()
	{
		_clearServerHandshake();
	}
	void PublicationSourceTrait::_onServerMessage(const spk::ReceivedMessage &received)
	{
		if (_protocol.kind(received.message) == Protocol::Kind::Hello)
		{
			_receiveHello(received.emitter, _protocol.decodeHandshake(received.message).token);
		}
		else if (auto peer = peerID(received.emitter))
		{
			(void)receiveMessage(*peer, received.message);
		}
	}
	bool PublicationSourceTrait::_sendMessage(PeerID peer, const spk::Message &message)
	{
		if (!isBound())
		{
			throw spk::Exception("Publication source has no transport");
		}
		const auto connection = _peerConnection(peer);
		return connection && _sendTo(*connection, message);
	}
	void PublicationSourceTrait::_requestObject(PeerID peer, const Request &request)
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
	PublicationSourceTrait::PublicationSourceTrait(spk::Message::Type type, Configuration configuration, std::size_t maximumBytes) :
		PublicationTrait(configuration),
		ObjectRequestHandlerTrait(configuration.maximumObjects),
		_protocol(type, maximumBytes)
	{
	}
	void PublicationSourceTrait::bind(spk::Server &server)
	{
		ServerBindingTrait::bind(server, _protocol.type());
	}
	std::optional<PeerID> PublicationSourceTrait::peerID(spk::ConnectionID connection) const
	{
		return _handshakePeerID(connection);
	}
	bool PublicationSourceTrait::receiveMessage(PeerID peer, const spk::Message &message)
	{
		_requirePublicationIdle();
		return receiveRequest(peer, _protocol.decodeRequest(message));
	}
	bool PublicationSourceTrait::acceptRequest(PeerID peer, const Request &request)
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
	bool PublicationSourceTrait::fulfillRequest(PeerID peer, const Request &request, spk::Message payload)
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
	bool PublicationSourceTrait::rejectRequest(PeerID peer, const Request &request)
	{
		OperationGuard guard(_publicationOperation());
		if (!_isCurrentRequest(peer, request) || !_sendMessage(peer, _protocol.encode(request, Protocol::Kind::Rejected)))
		{
			return false;
		}
		_completeRequest(peer, request.object);
		return true;
	}
}
