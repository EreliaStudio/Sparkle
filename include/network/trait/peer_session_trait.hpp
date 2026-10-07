#pragma once
#include "network/replication/types.hpp"
#include <map>
#include <optional>

namespace spk::Network
{
	class PeerSessionTrait
	{
		std::map<PeerID, SessionID> _sessions;
		std::size_t _maximumPeers;

	protected:
		[[nodiscard]] SessionID _openSession(PeerID peer);
		void _closeSession(PeerID peer);
		[[nodiscard]] std::optional<SessionID> _findSession(PeerID peer) const;
		[[nodiscard]] SessionID _peerSession(PeerID peer) const;

	public:
		explicit PeerSessionTrait(std::size_t maximumPeers = 256);
		PeerSessionTrait(const PeerSessionTrait &) = delete;
		PeerSessionTrait &operator=(const PeerSessionTrait &) = delete;
		virtual ~PeerSessionTrait() = default;
	};
}
