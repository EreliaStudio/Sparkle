#pragma once
#include "network/replication/protocol.hpp"
#include "object_request_handler_trait.hpp"
#include "publication_trait.hpp"
#include "server_binding_trait.hpp"
#include <map>
namespace spk::Network
{
	// Complete server replication channel; lower traits also support custom transports.
	template <typename State, typename Codec>
	class PublicationSourceTrait : public PublicationTrait<State>, protected ObjectRequestHandlerTrait, protected ServerBindingTrait
	{
		struct Connection
		{
			PeerID peer;
			SessionID token;
		};
		Protocol<State, Codec> _protocol;
		std::map<spk::ConnectionID, Connection> _connections;
		void _requireServerIdle() const override
		{
			this->_requirePublicationIdle();
		}
		void _requireRequestHandlerIdle() const override
		{
			this->_requirePublicationIdle();
		}
		std::optional<SessionID> _requestSession(PeerID peer) const override
		{
			return this->_findSession(peer);
		}
		void _onPeerClosed(PeerID peer) override
		{
			_cancelPeerRequests(peer);
		}
		void _onObjectForgotten(PeerID peer, ObjectID object) override
		{
			_completeRequest(peer, object);
		}
		void _onObjectDestroyed(ObjectID object) override
		{
			_cancelObjectRequests(object);
		}
		bool _sendUpdate(PeerID peer, const Update<State> &update, spk::Message::RequestID requestID) override
		{
			return _sendMessage(peer, _protocol.encode(update, requestID));
		}
		void _onServerConnectionClosed(spk::ConnectionID connection) override
		{
			auto found = _connections.find(connection);
			if (found != _connections.end())
			{
				this->closePeer(found->second.peer);
				_connections.erase(found);
			}
		}
		void _onServerUnbinding() override
		{
			for (const auto &[id, connection] : _connections)
			{
				this->closePeer(connection.peer);
			}
			_connections.clear();
		}
		void _receiveHello(spk::ConnectionID connection, const spk::Message &message)
		{
			const auto hello = _protocol.decodeHandshake(message);
			auto found = _connections.find(connection);
			if (found != _connections.end() && (found->second.token != hello.token || !this->_findSession(found->second.peer)))
			{
				this->closePeer(found->second.peer);
				_connections.erase(found);
			}
			if (!_connections.contains(connection))
			{
				const auto peer = PeerID::generate();
				(void)this->openPeer(peer);
				try
				{
					_connections.emplace(connection, Connection{peer, hello.token});
				} catch (...)
				{
					this->closePeer(peer);
					throw;
				}
			}
			const auto peer = _connections.at(connection).peer;
			(void)_sendTo(connection, _protocol.encodeHandshake(hello.token, this->_peerSession(peer)));
		}
		void _onServerMessage(const spk::ReceivedMessage &received) override
		{
			if (_protocol.kind(received.message) == Protocol<State, Codec>::Kind::Hello)
			{
				_receiveHello(received.emitter, received.message);
			}
			else if (auto peer = peerID(received.emitter))
			{
				(void)receiveMessage(*peer, received.message);
			}
		}

	protected:
		// Acceptance by one ordered transport, not a remote acknowledgement.
		[[nodiscard]] virtual bool _sendMessage(PeerID peer, const spk::Message &message)
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
		void _requestObject(PeerID peer, const Request &request) override
		{
			if (this->_hasSnapshot(request.object))
			{
				(void)acceptRequest(peer, request);
			}
			else
			{
				(void)rejectRequest(peer, request);
			}
		}

	public:
		using Configuration = typename PublicationTrait<State>::Configuration;
		explicit PublicationSourceTrait(spk::Message::Type type, Configuration configuration = {}, std::size_t maximumBytes = 2 * 1024 * 1024) :
			PublicationTrait<State>(configuration),
			ObjectRequestHandlerTrait(configuration.maximumObjects),
			_protocol(type, maximumBytes)
		{
		}
		~PublicationSourceTrait() override = default;
		void bind(spk::Server &server)
		{
			ServerBindingTrait::bind(server, _protocol.type());
		}
		using ServerBindingTrait::isBound;
		using ServerBindingTrait::unbind;
		[[nodiscard]] std::optional<PeerID> peerID(spk::ConnectionID connection) const
		{
			auto found = _connections.find(connection);
			return found == _connections.end() || !this->_findSession(found->second.peer) ? std::nullopt : std::optional{found->second.peer};
		}
		[[nodiscard]] bool receiveMessage(PeerID peer, const spk::Message &message)
		{
			this->_requirePublicationIdle();
			return receiveRequest(peer, _protocol.decodeRequest(message));
		}
		[[nodiscard]] bool acceptRequest(PeerID peer, const Request &request)
		{
			OperationGuard guard(this->_publicationOperation());
			if (!_isCurrentRequest(peer, request))
			{
				return false;
			}
			this->_follow(peer, request.object, request.id);
			_completeRequest(peer, request.object);
			return true;
		}
		[[nodiscard]] bool fulfillRequest(PeerID peer, const Request &request, State state)
		{
			OperationGuard guard(this->_publicationOperation());
			if (!_isCurrentRequest(peer, request))
			{
				return false;
			}
			this->_room(peer, request.object);
			this->_publish(request.object, std::move(state));
			this->_follow(peer, request.object, request.id);
			_completeRequest(peer, request.object);
			return true;
		}
		[[nodiscard]] bool rejectRequest(PeerID peer, const Request &request)
		{
			OperationGuard guard(this->_publicationOperation());
			if (!_isCurrentRequest(peer, request) || !_sendMessage(peer, _protocol.encode(request, Protocol<State, Codec>::Kind::Rejected)))
			{
				return false;
			}
			_completeRequest(peer, request.object);
			return true;
		}
	};
}
