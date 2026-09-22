#pragma once

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "design_pattern/trait/updatable_by_device_trait.hpp"

namespace spk
{
	class RenderingObjectTrait : public EventDispatcher,
								 public UpdatableByDeviceTrait,
								 public RenderSnapshotContributorTrait
	{
	protected:
		void _propagateEvent(
			const std::function<void(EventDispatcher *)> &) override
		{
		}

	public:
		~RenderingObjectTrait() override = default;
	};
}
