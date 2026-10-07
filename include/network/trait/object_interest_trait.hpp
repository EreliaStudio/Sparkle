#pragma once
#include "network/replication/sequence.hpp"
#include "network/replication/types.hpp"
#include <map>
#include <vector>
namespace spk::Network
{
	class ObjectInterestTrait
	{
		std::map<PeerID, std::map<ObjectID, std::uint64_t>> _interest;
		Sequence _trackingIDs;

	protected:
		[[nodiscard]] bool _follows(PeerID peer, ObjectID object) const;
		[[nodiscard]] std::uint64_t _track(PeerID peer, ObjectID object);
		[[nodiscard]] std::uint64_t _trackingID(PeerID peer, ObjectID object) const;
		[[nodiscard]] std::vector<PeerID> _followers(ObjectID object) const;
		void _forgetInterest(PeerID peer, ObjectID object);
		void _forgetPeerInterest(PeerID peer);
		void _forgetObjectInterest(ObjectID object);

	public:
		ObjectInterestTrait() = default;
		ObjectInterestTrait(const ObjectInterestTrait &) = delete;
		ObjectInterestTrait &operator=(const ObjectInterestTrait &) = delete;
		virtual ~ObjectInterestTrait() = default;
	};
}
