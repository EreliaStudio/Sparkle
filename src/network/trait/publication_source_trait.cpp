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
	void PublicationSourceTrait::_onServerConnectionClosed(spk::ConnectionID connection)
	{
		auto found = _connections.find(connection);
		if (found != _connections.end())
		{
			closePeer(found->second.peer);
			_connections.erase(found);
		}
	}
	void PublicationSourceTrait::_onServerUnbinding()
	{
		for (const auto &[id, connection] : _connections)
		{
			closePeer(connection.peer);
		}
		_connections.clear();
	}
	void PublicationSourceTrait::_receiveHello(spk::ConnectionID connection, const spk::Message &message)
	{
		const auto hello = _protocol.decodeHandshake(message);
		auto found = _connections.find(connection);
		if (found != _connections.end() && (found->second.token != hello.token || !_findSession(found->second.peer)))
		{
			closePeer(found->second.peer);
			_connections.erase(found);
		}
		if (!_connections.contains(connection))
		{
			const auto peer = PeerID::generate();
			(void)openPeer(peer);
			try
			{
				_connections.emplace(connection, Connection{peer, hello.token});
			} catch (...)
			{
				closePeer(peer);
				throw;
			}
		}
		const auto peer = _connections.at(connection).peer;
		(void)_sendTo(connection, _protocol.encodeHandshake(hello.token, _peerSession(peer)));
	}
	void PublicationSourceTrait::_onServerMessage(const spk::ReceivedMessage &received)
	{
		if (_protocol.kind(received.message) == Protocol::Kind::Hello)
		{
			_receiveHello(received.emitter, received.message);
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
		for (const auto &[connection, binding] : _connections)
		{
			if (binding.peer == peer)
			{
				return _sendTo(connection, message);
			}
		}
		return false;
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
		auto found = _connections.find(connection);
		return found == _connections.end() || !_findSession(found->second.peer) ? std::nullopt : std::optional{found->second.peer};
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
