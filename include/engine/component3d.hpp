#pragma once

#include "engine/entity3d.hpp"
#include "engine/component.hpp"

#include <string>

namespace spk
{
	class Component3D : public Component,
						  public Registry<Component3D, Engine *>::Object
	{
	public:
		Component3D(
			const std::string &name = "Unnamed component",
			Entity3D *owner = nullptr);
		explicit Component3D(Entity3D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity3D *owner() override;
		[[nodiscard]] const Entity3D *owner() const override;
	};
}
