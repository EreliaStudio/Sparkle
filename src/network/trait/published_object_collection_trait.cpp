#include "network/trait/published_object_collection_trait.hpp"
namespace spk::Network
{
	void PublishedObjectCollectionTrait::_publishObserved(ObjectID id, spk::Message payload)
	{
		_publish(id, std::move(payload));
	}
	void PublishedObjectCollectionTrait::_eraseSnapshot(ObjectID id)
	{
		PublicationSnapshotTrait::_eraseSnapshot(id);
		_unregisterObject(id);
	}
	PublishedObjectCollectionTrait::PublishedObjectCollectionTrait(std::size_t maximumObjects) :
		PublicationSnapshotTrait(maximumObjects)
	{
	}
}
