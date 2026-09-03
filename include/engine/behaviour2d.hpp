#pragma once

#include "engine/entity2d.hpp"
#include "engine/behaviour.hpp"

#include <string>

namespace spk
{
	class Behaviour2D : public Behaviour,
						public spk::Registry<Engine *, Behaviour2D>::Object
	{
	public:
		Behaviour2D(
			const std::string &name = "Unnamed behaviour",
			Entity2D *owner = nullptr);
		explicit Behaviour2D(Entity2D *owner);

		void attach(Entity *owner) override;

		[[nodiscard]] Entity2D *owner() override;
		[[nodiscard]] const Entity2D *owner() const override;
	};
}