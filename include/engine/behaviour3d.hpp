#pragma once

#include "engine/entity3d.hpp"
#include "engine/behaviour.hpp"

#include <string>

namespace spk
{
	class Behaviour3D : public Behaviour,
						public spk::Registry<Engine *, Behaviour3D>::Object
	{
	public:
		Behaviour3D(
			const std::string &name = "Unnamed behaviour",
			Entity3D *owner = nullptr);
		explicit Behaviour3D(Entity3D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity3D *owner() override;
		[[nodiscard]] const Entity3D *owner() const override;
	};
}