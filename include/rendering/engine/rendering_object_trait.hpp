#pragma once

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "design_pattern/trait/updatable_by_device_trait.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderingObjectTrait : public EventDispatcher,
								 public UpdatableByDeviceTrait,
								 public RenderSnapshotContributorTrait,
								 public EngineView::Contributor
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
