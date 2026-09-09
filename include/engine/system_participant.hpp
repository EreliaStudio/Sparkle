#pragma once

#include "design_pattern/trait/geometry_state_trait.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "engine/entity_attachment.hpp"
#include "engine/registry.hpp"
#include "engine/system.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"

#include <string>

namespace spk
{
	class Entity;

	class System::Participant : public EntityAttachment,
								public spk::Registry<System::Participant, Engine *>::Object,
								public GeometryStateTrait,
								public RenderSnapshotContributorTrait
	{
	protected:
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _onGeometryChange(const spk::Rect2D &geometry) override;

	public:
		Participant(
			const std::string &name = "Unnamed participant",
			Entity *owner = nullptr);
		explicit Participant(Entity *owner);

		~Participant() override = default;
	};
}
