#pragma once

#include "engine/entity.hpp"

namespace spk
{
	class Transform2D;

	class Entity2D : public Entity,
					 public Registry<Engine *, Entity2D>::Object
	{
	private:
		Transform2D &_transform;
		InherenceTrait<Entity>::OnParentEditionContract _onParentEditionContract;

	public:
		Entity2D(const std::string &name, Entity *parent = nullptr);

		[[nodiscard]] Transform2D &transform();
		[[nodiscard]] const Transform2D &transform() const;
	};
}
