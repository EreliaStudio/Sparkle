#pragma once
#include "network/replication/protocol.hpp"
#include "object_request_handler_trait.hpp"
#include "publication_trait.hpp"
#include "server_binding_trait.hpp"
#include "server_handshake_trait.hpp"
#include <optional>
namespace spk::Network
{
	// Complete server replication channel; lower traits also support custom transports.
	class PublicationSourceTrait : public PublicationTrait, protected ObjectRequestHandlerTrait, protected ServerBindingTrait, protected ServerHandshakeTrait
	{
		Protocol _protocol;
		SessionID _openHandshakePeer(PeerID peer) override;
		void _closeHandshakePeer(PeerID peer) override;
		std::optional<SessionID> _findHandshakeSession(PeerID peer) const override;
		void _sendHandshakeSession(spk::ConnectionID connection, SessionID token, SessionID session) override;
		void _requireServerIdle() const override;
		void _requireRequestHandlerIdle() const override;
		std::optional<SessionID> _requestSession(PeerID peer) const override;
		void _onPeerClosed(PeerID peer) override;
		void _onObjectForgotten(PeerID peer, ObjectID object) override;
		void _onObjectDestroyed(ObjectID object) override;
		bool _sendUpdate(PeerID peer, const Update<spk::Message> &update, spk::Message::RequestID requestID) override;
		void _onServerConnectionClosed(spk::ConnectionID connection) override;
		void _onServerUnbinding() override;
		void _onServerMessage(const spk::ReceivedMessage &received) override;

	protected:
		// Acceptance by one ordered transport, not a remote acknowledgement.
		[[nodiscard]] virtual bool _sendMessage(PeerID peer, const spk::Message &message);
		void _requestObject(PeerID peer, const Request &request) override;

	public:
		using Configuration = PublicationTrait::Configuration;
		explicit PublicationSourceTrait(spk::Message::Type type, Configuration configuration = {}, std::size_t maximumBytes = 2 * 1024 * 1024);
		~PublicationSourceTrait() override = default;
		void bind(spk::Server &server);
		using ServerBindingTrait::isBound;
		using ServerBindingTrait::unbind;
		[[nodiscard]] std::optional<PeerID> peerID(spk::ConnectionID connection) const;
		[[nodiscard]] bool receiveMessage(PeerID peer, const spk::Message &message);
		[[nodiscard]] bool acceptRequest(PeerID peer, const Request &request);
		[[nodiscard]] bool fulfillRequest(PeerID peer, const Request &request, spk::Message payload);
		[[nodiscard]] bool rejectRequest(PeerID peer, const Request &request);
	};
}
