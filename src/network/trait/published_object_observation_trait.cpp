#include "network/trait/published_object_observation_trait.hpp"
#include "exception.hpp"
namespace spk::Network
{
	void PublishedObjectObservationTrait::_captureChanges()
	{
		for (auto &[id, object] : _observations)
		{
			if (object.lifetime.expired() || object.edition == object.instance->version())
			{
				continue;
			}
			const auto edition = object.instance->version();
			_publishObserved(id, object.instance->captureNetworkState());
			object.edition = edition;
		}
	}
	void PublishedObjectObservationTrait::_registerObject(ObjectID id, PublishableTrait &instance)
	{
		if (id.isNull())
		{
			throw spk::Exception("Null network object identity");
		}
		auto found = _observations.find(id);
		if (found != _observations.end() && !found->second.lifetime.expired())
		{
			throw spk::Exception("Network object already registered");
		}
		const auto edition = instance.version();
		_publishObserved(id, instance.captureNetworkState());
		_observations.insert_or_assign(id, Observation{&instance, instance._lifetime, edition});
	}
	void PublishedObjectObservationTrait::_unregisterObject(ObjectID id)
	{
		_observations.erase(id);
	}
}
