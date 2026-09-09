#pragma once

#include "design_pattern/trait/geometry_state_trait.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "engine/entity_attachment.hpp"
#include "engine/registry.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

#include <string>

namespace spk
{
	class Entity;
	class Engine;

	class Component : public EntityAttachment,
						public Registry<Component, Engine *>::Object,
						public GeometryStateTrait,
						public RenderSnapshotContributorTrait
	{
	protected:
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;

	public:
		Component(
			const std::string &name = "Unnamed component",
			Entity *owner = nullptr);
		explicit Component(Entity *owner);

		~Component() override = default;
	};
}
