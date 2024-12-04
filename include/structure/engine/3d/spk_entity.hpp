#pragma once

#include "structure/engine/spk_generic_entity.hpp"

#include "structure/system/event/spk_event.hpp"

#include "structure/engine/3d/spk_transform.hpp"
#include <set>
#include <span>
#include <string>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class Entity : public spk::GenericEntity
	{
	private:
		spk::SafePointer<Transform> _transform;
		Transform::Contract _ownerTransformEditionContract;

	public:
		Entity(const std::wstring &p_name, const spk::SafePointer<GenericEntity> &p_owner = nullptr);

		Entity();

		const spk::Vector3 &position() const;
		Transform &transform();
		const Transform &transform() const;
	};
}