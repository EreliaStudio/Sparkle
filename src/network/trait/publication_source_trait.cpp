#include "network/trait/publication_source_trait.hpp"

namespace spk::Network
{
	void PublicationSourceTrait::_requireServerIdle() const
	{
		_requirePublicationIdle();
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
	PublicationSourceTrait::PublicationSourceTrait(spk::Message::Type type, Configuration configuration, std::size_t maximumBytes) :
		PublicationRequestTrait(configuration),
		_protocol(type, maximumBytes)
	{
	}
	bool PublicationSourceTrait::_sendRejection(PeerID peer, const Request &request)
	{
		return _sendMessage(peer, _protocol.encode(request, Protocol::Kind::Rejected));
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
}
