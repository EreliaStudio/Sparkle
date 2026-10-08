#pragma once
#include "object_request_handler_trait.hpp"
#include "publication_trait.hpp"
namespace spk::Network
{
	// Coordinates acquisition requests with published state, independently of transport.
	class PublicationRequestTrait : public PublicationTrait, protected ObjectRequestHandlerTrait
	{
		void _requireRequestHandlerIdle() const override;
		std::optional<SessionID> _requestSession(PeerID peer) const override;
		void _onPeerClosed(PeerID peer) override;
		void _onObjectForgotten(PeerID peer, ObjectID object) override;
		void _onObjectDestroyed(ObjectID object) override;

	protected:
		[[nodiscard]] virtual bool _sendRejection(PeerID peer, const Request &request) = 0;
		void _requestObject(PeerID peer, const Request &request) override;

	public:
		PublicationRequestTrait();
		explicit PublicationRequestTrait(Configuration configuration);
		~PublicationRequestTrait() override = default;
		using ObjectRequestHandlerTrait::receiveRequest;
		[[nodiscard]] bool acceptRequest(PeerID peer, const Request &request);
		[[nodiscard]] bool fulfillRequest(PeerID peer, const Request &request, spk::Message payload);
		[[nodiscard]] bool rejectRequest(PeerID peer, const Request &request);
	};
}
