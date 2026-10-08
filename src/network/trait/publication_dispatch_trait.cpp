#include "network/trait/publication_dispatch_trait.hpp"
#include <algorithm>
#include <set>
namespace spk::Network
{
	void PublicationDispatchTrait::_capturePublicationChanges()
	{
	}
	void PublicationDispatchTrait::_openQueue(PeerID peer)
	{
		PublicationDeliveryTrait::_openQueue(peer);
		try
		{
			_schedulePeer(peer);
		} catch (...)
		{
			PublicationDeliveryTrait::_closeQueue(peer);
			throw;
		}
	}
	void PublicationDispatchTrait::_closeQueue(PeerID peer)
	{
		PublicationQueueTrait::_closeQueue(peer);
		_unschedulePeer(peer);
	}
	PublicationDispatchTrait::DispatchResult PublicationDispatchTrait::_dispatchPublication(Clock::time_point now, std::size_t maximumAttempts)
	{
		if (!_publicationDue(now) || maximumAttempts == 0 || !_hasScheduledPeers())
		{
			return {};
		}
		_capturePublicationChanges();
		_advancePublication(now);
		return _dispatchReady(maximumAttempts);
	}
	PublicationDispatchTrait::DispatchResult PublicationDispatchTrait::_dispatchReady(std::size_t maximumAttempts)
	{
		DispatchResult result;
		std::set<PeerID> skipped;
		for (std::size_t count = 0; count < maximumAttempts && skipped.size() < _scheduledPeerCount(); ++count)
		{
			const auto peer = _roundRobin.front();
			_roundRobin.pop_front();
			_roundRobin.push_back(peer);
			if (!skipped.contains(peer) && !_dispatchOne(peer, result))
			{
				skipped.insert(peer);
			}
		}
		return result;
	}
	PublicationDispatchTrait::PublicationDispatchTrait(std::size_t maximumPending, Clock::duration interval) :
		PublicationDeliveryTrait(maximumPending),
		PublicationCadenceTrait(interval)
	{
	}
}
