#include "network/trait/publication_snapshot_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	void PublicationSnapshotTrait::_beforeSnapshot(ObjectID)
	{
	}
	void PublicationSnapshotTrait::_onSnapshot(ObjectID)
	{
	}
	bool PublicationSnapshotTrait::_hasSnapshot(ObjectID id) const
	{
		return _snapshots.contains(id);
	}
	const PublicationSnapshotTrait::Snapshot &PublicationSnapshotTrait::_snapshot(ObjectID id) const
	{
		auto found = _snapshots.find(id);
		if (found == _snapshots.end())
		{
			throw spk::Exception("Unknown replication object");
		}
		return found->second;
	}
	void PublicationSnapshotTrait::_publish(ObjectID id, spk::Message payload)
	{
		if (id.isNull() || (!_snapshots.contains(id) && _snapshots.size() >= _maximumObjects))
		{
			throw spk::Exception("Invalid object identity or object limit reached");
		}
		_beforeSnapshot(id);
		const auto revision = _revisions.next();
		auto &object = _snapshots[id];
		object.payload = std::move(payload);
		object.revision = revision;
		_onSnapshot(id);
	}
	void PublicationSnapshotTrait::_eraseSnapshot(ObjectID id)
	{
		_snapshots.erase(id);
	}
	PublicationSnapshotTrait::PublicationSnapshotTrait(std::size_t maximumObjects) :
		_maximumObjects(maximumObjects)
	{
		if (maximumObjects == 0)
		{
			throw spk::Exception("Invalid object capacity");
		}
	}
}
