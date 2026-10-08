#pragma once
#include "network/replication/types.hpp"
#include <cstddef>
#include <deque>

namespace spk::Network
{
	// Owns fair rotation independently of queued payloads and transport.
	class PublicationFairnessTrait
	{
		std::deque<PeerID> _roundRobin;

	protected:
		void _schedulePeer(PeerID peer);
		void _unschedulePeer(PeerID peer);
		[[nodiscard]] bool _hasScheduledPeers() const noexcept;
		[[nodiscard]] std::size_t _scheduledPeerCount() const noexcept;
		[[nodiscard]] PeerID _nextScheduledPeer();

	public:
		virtual ~PublicationFairnessTrait() = default;
	};
}
