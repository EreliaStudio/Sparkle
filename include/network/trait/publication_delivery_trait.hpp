#pragma once
#include "publication_queue_trait.hpp"

namespace spk::Network
{
	// Attempts one queued delivery; failed sends never acknowledge the queued item.
	class PublicationDeliveryTrait : protected PublicationQueueTrait
	{
	public:
		struct DispatchResult
		{
			std::size_t sent = 0, blocked = 0, errors = 0;
		};

	protected:
		[[nodiscard]] virtual bool _sendUpdate(PeerID, const Update<spk::Message> &, spk::Message::RequestID) = 0;
		[[nodiscard]] bool _dispatchOne(PeerID peer, DispatchResult &result);

	public:
		explicit PublicationDeliveryTrait(std::size_t maximumPending = 16384);
		~PublicationDeliveryTrait() override = default;
	};
}
