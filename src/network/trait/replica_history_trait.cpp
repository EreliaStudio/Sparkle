#include "network/trait/replica_history_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	ReplicaHistoryTrait::ReplicaHistoryTrait(std::size_t maximumTracked) :
		_maximumTracked(maximumTracked)
	{
		if (maximumTracked == 0)
		{
			throw spk::Exception("Invalid replica capacity");
		}
	}
	bool ReplicaHistoryTrait::_obsolete(const Tracking &tracked, std::uint64_t identity, std::uint64_t revision, Edit edit)
	{
		if (identity != tracked.identity)
		{
			return identity < tracked.identity;
		}
		return !tracked.active || (edit == Edit::Set && revision <= tracked.revision);
	}
	bool ReplicaHistoryTrait::_applyTracked(ObjectID object, std::uint64_t identity, std::uint64_t revision, Edit edit, const std::function<void()> &apply)
	{
		if (object.isNull() || identity == 0 || revision == 0 || edit > Edit::Destroy)
		{
			throw spk::Exception("Invalid replica history metadata");
		}
		auto found = _tracking.find(object);
		if (found != _tracking.end() && _obsolete(found->second, identity, revision, edit))
		{
			return false;
		}
		if (found == _tracking.end() && _tracking.size() >= _maximumTracked)
		{
			throw spk::Exception("Tracking history full; reset session");
		}
		auto [entry, inserted] = _tracking.try_emplace(object);
		try
		{
			apply();
		} catch (...)
		{
			if (inserted)
			{
				_tracking.erase(entry);
			}
			throw;
		}
		entry->second = {identity, revision, edit == Edit::Set};
		return true;
	}
	bool ReplicaHistoryTrait::_tracksActive(ObjectID object, std::uint64_t identity) const
	{
		auto found = _tracking.find(object);
		return found != _tracking.end() && found->second.active && found->second.identity == identity;
	}
	std::vector<ObjectID> ReplicaHistoryTrait::_activeReplicas() const
	{
		std::vector<ObjectID> result;
		for (const auto &[object, tracking] : _tracking)
		{
			if (tracking.active)
			{
				result.push_back(object);
			}
		}
		return result;
	}
	void ReplicaHistoryTrait::_clearHistory()
	{
		_tracking.clear();
	}
}
