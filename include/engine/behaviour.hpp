#pragma once

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/geometry_state_trait.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "engine/entity_attachment.hpp"

#include <string>

namespace spk
{
	struct UpdateContext;
	class Entity;

	class Behaviour : public EntityAttachment,
					  public EventDispatcher,
					  public GeometryStateTrait,
					  public UpdatableTrait,
					  public RenderSnapshotContributorTrait
	{
	protected:
		[[nodiscard]] bool _isAcceptingEvent() const override;
		[[nodiscard]] bool _canUpdate() const override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;

	public:
		Behaviour(
			const std::string &name = "Unnamed behaviour",
			Entity *owner = nullptr);
		explicit Behaviour(Entity *owner);

		~Behaviour() override = default;
	};
}
