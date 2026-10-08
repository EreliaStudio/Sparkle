#pragma once
#include "publication_snapshot_trait.hpp"
#include "published_object_observation_trait.hpp"
namespace spk::Network
{
	// Composes live-object observation with independently retained publication state.
	class PublishedObjectCollectionTrait : protected PublicationSnapshotTrait, protected PublishedObjectObservationTrait
	{
		void _publishObserved(ObjectID id, spk::Message payload) override;

	protected:
		void _eraseSnapshot(ObjectID id);

	public:
		explicit PublishedObjectCollectionTrait(std::size_t maximumObjects = 16384);
		~PublishedObjectCollectionTrait() override = default;
	};
}
