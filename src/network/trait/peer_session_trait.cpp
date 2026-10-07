#include "network/trait/peer_session_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	PeerSessionTrait::PeerSessionTrait(std::size_t maximumPeers) :
		_maximumPeers(maximumPeers)
	{
		if (maximumPeers == 0)
		{
			throw spk::Exception("Invalid peer capacity");
		}
	}
	SessionID PeerSessionTrait::_openSession(PeerID peer)
	{
		if (peer.isNull() || _sessions.contains(peer) || _sessions.size() >= _maximumPeers)
		{
			throw spk::Exception("Invalid, duplicate or excess replication peer");
		}
		return _sessions.emplace(peer, SessionID::generate()).first->second;
	}
	void PeerSessionTrait::_closeSession(PeerID peer)
	{
		_sessions.erase(peer);
	}
	std::optional<SessionID> PeerSessionTrait::_findSession(PeerID peer) const
	{
		const auto found = _sessions.find(peer);
		return found == _sessions.end() ? std::nullopt : std::optional{found->second};
	}
	SessionID PeerSessionTrait::_peerSession(PeerID peer) const
	{
		auto session = _findSession(peer);
		if (!session)
		{
			throw spk::Exception("Unknown replication peer");
		}
		return *session;
	}
}
