#include "network/trait/published_object_collection_trait.hpp"

namespace spk::Network
{
	void PublishedObjectCollectionTrait::_beforeSnapshot(ObjectID)
	{
	}
	void PublishedObjectCollectionTrait::_onSnapshot(ObjectID)
	{
	}
	bool PublishedObjectCollectionTrait::_hasSnapshot(ObjectID id) const
	{
		return _objects.contains(id);
	}
	const PublishedObjectCollectionTrait::RegisteredObject &PublishedObjectCollectionTrait::_snapshot(ObjectID id) const
	{
		auto found = _objects.find(id);
		if (found == _objects.end())
		{
			throw spk::Exception("Unknown replication object");
		}
		return found->second;
	}
	void PublishedObjectCollectionTrait::_publish(ObjectID id, spk::Message payload)
	{
		if (id.isNull() || (!_objects.contains(id) && _objects.size() >= _maximumObjects))
		{
			throw spk::Exception("Invalid object identity or object limit reached");
		}
		_beforeSnapshot(id);
		const auto revision = _revisions.next();
		auto &object = _objects[id];
		object.payload = std::move(payload);
		object.revision = revision;
		_onSnapshot(id);
	}
	void PublishedObjectCollectionTrait::_captureChanges()
	{
		for (auto &[id, object] : _objects)
		{
			if (object.lifetime.expired() || object.edition == object.instance->version())
			{
				continue;
			}
			const auto edition = object.instance->version();
			_publish(id, object.instance->captureNetworkState());
			object.edition = edition;
		}
	}
	void PublishedObjectCollectionTrait::_registerObject(ObjectID id, PublishableTrait &instance)
	{
		auto found = _objects.find(id);
		if (found != _objects.end() && !found->second.lifetime.expired())
		{
			throw spk::Exception("Network object already registered");
		}
		const auto edition = instance.version();
		_publish(id, instance.captureNetworkState());
		auto &object = _objects.at(id);
		object.instance = &instance;
		object.lifetime = instance._lifetime;
		object.edition = edition;
	}
	void PublishedObjectCollectionTrait::_unregisterObject(ObjectID id)
	{
		if (auto found = _objects.find(id); found != _objects.end())
		{
			found->second.lifetime.reset();
			found->second.instance = nullptr;
		}
	}
	void PublishedObjectCollectionTrait::_eraseSnapshot(ObjectID id)
	{
		_objects.erase(id);
	}
	PublishedObjectCollectionTrait::PublishedObjectCollectionTrait(std::size_t maximumObjects) :
		_maximumObjects(maximumObjects)
	{
		if (maximumObjects == 0)
		{
			throw spk::Exception("Invalid object capacity");
		}
	}
}
