#pragma once

#include "structure/engine/spk_generic_entity.hpp"

#include "structure/system/event/spk_event.hpp"

#include "structure/engine/2d/spk_transform_2d.hpp"
#include <set>
#include <span>
#include <string>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class Entity2D : public spk::GenericEntity
	{
	private:
		spk::SafePointer<Transform2D> _transform;
		Transform2D::Contract _ownerTransformEditionContract;

	public:
		Entity2D(const std::wstring &p_name, const spk::SafePointer<GenericEntity> &p_owner = nullptr);

		Entity2D();

		const spk::Vector2 &position() const;
		Transform2D &transform();
		const Transform2D &transform() const;
	};
}