#include "network/trait/publication_fairness_trait.hpp"
#include "exception.hpp"
#include <algorithm>

namespace spk::Network
{
	void PublicationFairnessTrait::_schedulePeer(PeerID peer)
	{
		_roundRobin.push_back(peer);
	}
	void PublicationFairnessTrait::_unschedulePeer(PeerID peer)
	{
		std::erase(_roundRobin, peer);
	}
	bool PublicationFairnessTrait::_hasScheduledPeers() const noexcept
	{
		return !_roundRobin.empty();
	}
	std::size_t PublicationFairnessTrait::_scheduledPeerCount() const noexcept
	{
		return _roundRobin.size();
	}
	PeerID PublicationFairnessTrait::_nextScheduledPeer()
	{
		if (_roundRobin.empty())
		{
			throw spk::Exception("No scheduled publication peers");
		}
		const auto peer = _roundRobin.front();
		_roundRobin.pop_front();
		_roundRobin.push_back(peer);
		return peer;
	}
}
