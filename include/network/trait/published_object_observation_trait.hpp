#pragma once
#include "network/replication/types.hpp"
#include "publishable_trait.hpp"
#include <map>
namespace spk::Network
{
	// Each observer independently remembers the last successfully captured version.
	class PublishedObjectObservationTrait
	{
		struct Observation
		{
			PublishableTrait *instance;
			std::weak_ptr<void> lifetime;
			spk::VersionedTrait::Version edition;
		};
		std::map<ObjectID, Observation> _observations;

	protected:
		virtual void _publishObserved(ObjectID id, spk::Message payload) = 0;
		void _captureChanges();
		void _registerObject(ObjectID id, PublishableTrait &instance);
		void _unregisterObject(ObjectID id);

	public:
		PublishedObjectObservationTrait() = default;
		virtual ~PublishedObjectObservationTrait() = default;
		PublishedObjectObservationTrait(const PublishedObjectObservationTrait &) = delete;
		PublishedObjectObservationTrait &operator=(const PublishedObjectObservationTrait &) = delete;
	};
}
