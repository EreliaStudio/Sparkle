#include "network/trait/object_interest_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	bool ObjectInterestTrait::_follows(PeerID peer, ObjectID object) const
	{
		auto found = _interest.find(peer);
		return found != _interest.end() && found->second.contains(object);
	}
	std::uint64_t ObjectInterestTrait::_track(PeerID peer, ObjectID object)
	{
		if (peer.isNull() || object.isNull())
		{
			throw spk::Exception("Null interest identity");
		}
		if (_follows(peer, object))
		{
			return _trackingID(peer, object);
		}
		const auto tracking = _trackingIDs.next();
		_interest[peer].emplace(object, tracking);
		return tracking;
	}
	std::uint64_t ObjectInterestTrait::_trackingID(PeerID peer, ObjectID object) const
	{
		if (!_follows(peer, object))
		{
			throw spk::Exception("Unknown object interest");
		}
		return _interest.at(peer).at(object);
	}
	std::vector<PeerID> ObjectInterestTrait::_followers(ObjectID object) const
	{
		std::vector<PeerID> result;
		for (const auto &[peer, objects] : _interest)
		{
			if (objects.contains(object))
			{
				result.push_back(peer);
			}
		}
		return result;
	}
	void ObjectInterestTrait::_forgetInterest(PeerID peer, ObjectID object)
	{
		auto found = _interest.find(peer);
		if (found == _interest.end())
		{
			return;
		}
		found->second.erase(object);
		if (found->second.empty())
		{
			_interest.erase(found);
		}
	}
	void ObjectInterestTrait::_forgetPeerInterest(PeerID peer)
	{
		_interest.erase(peer);
	}
	void ObjectInterestTrait::_forgetObjectInterest(ObjectID object)
	{
		for (auto peer : _followers(object))
		{
			_forgetInterest(peer, object);
		}
	}
}
