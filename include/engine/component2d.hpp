#pragma once

#include "engine/entity2d.hpp"
#include "engine/component.hpp"

#include <string>

namespace spk
{
	class Component2D : public Component,
						  public Registry<Component2D, Engine *>::Object
	{
	public:
		Component2D(
			const std::string &name = "Unnamed component",
			Entity2D *owner = nullptr);
		explicit Component2D(Entity2D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity2D *owner() override;
		[[nodiscard]] const Entity2D *owner() const override;
	};
}
