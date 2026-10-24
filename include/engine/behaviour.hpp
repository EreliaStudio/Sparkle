#pragma once

#include <string>

#include "design_pattern/trait/geometry_state_trait.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "engine/entity_attachment.hpp"

namespace spk
{
	struct UpdateContext;
	class Entity;

	class Behaviour : public EntityAttachment,
					  public GeometryStateTrait,
					  public UpdatableTrait
	{
	protected:
		[[nodiscard]] bool _canUpdate() const override;

	public:
		Behaviour(
			const std::string &name = "Unnamed behaviour",
			Entity *owner = nullptr);
		explicit Behaviour(Entity *owner);

		~Behaviour() override = default;
	};
}
