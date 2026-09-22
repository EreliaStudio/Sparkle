#pragma once

#include <string>

#include "design_pattern/trait/geometry_state_trait.hpp"
#include "engine/entity_attachment.hpp"
#include "engine/registry.hpp"
#include "math/rect2d.hpp"

namespace spk
{
	class Entity;
	class Engine;

	class Component : public EntityAttachment,
					  public Registry<Component, Engine *>::Object,
					  public GeometryStateTrait
	{
	public:
		Component(
			const std::string &name = "Unnamed component",
			Entity *owner = nullptr);
		explicit Component(Entity *owner);

		~Component() override = default;
	};
}
