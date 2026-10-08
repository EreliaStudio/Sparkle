#include "network/trait/publication_delivery_trait.hpp"

namespace spk::Network
{
	PublicationDeliveryTrait::PublicationDeliveryTrait(std::size_t maximumPending) :
		PublicationQueueTrait(maximumPending)
	{
	}
	bool PublicationDeliveryTrait::_dispatchOne(PeerID peer, DispatchResult &result)
	{
		const auto *pending = _front(peer);
		if (pending == nullptr)
		{
			return false;
		}
		try
		{
			if (_sendUpdate(peer, pending->update, pending->requestID))
			{
				_pop(peer);
				++result.sent;
				return true;
			}
		} catch (...)
		{
			++result.errors;
		}
		++result.blocked;
		return false;
	}
}
