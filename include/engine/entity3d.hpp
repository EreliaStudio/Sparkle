#pragma once

#include "engine/entity.hpp"

namespace spk
{
	class Transform3D;

	class Entity3D : public Entity,
					 public Registry<Engine *, Entity3D>::Object
	{
	private:
		Transform3D &_transform;
		InherenceTrait<Entity>::OnParentEditionContract _onParentEditionContract;

	public:
		Entity3D(const std::string &name, Entity *parent = nullptr);

		[[nodiscard]] Transform3D &transform();
		[[nodiscard]] const Transform3D &transform() const;
	};
}
