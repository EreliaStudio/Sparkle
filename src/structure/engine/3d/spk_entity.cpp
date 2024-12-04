#include "structure/engine/3d/spk_entity.hpp"
#include "structure/engine/3d/spk_collider.hpp"
#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	Entity::Entity(const std::wstring &p_name, const spk::SafePointer<GenericEntity> &p_owner) :
		spk::GenericEntity(p_name),
		_transform(addComponent<Transform>())
	{
		_ownerTransformEditionContract = _transform->addOnEditionCallback(
			[&]()
			{
				for (auto &child : children())
				{
					child.upCast<spk::Entity>()->transform()._updateModel();
				}
			});
	}

	Entity::Entity() :
		Entity(L"Unnamed entity")
	{
	}

	const spk::Vector3 &Entity::position() const
	{
		return (_transform->position());
	}

	Transform &Entity::transform()
	{
		return (*_transform);
	}

	const Transform &Entity::transform() const
	{
		return (*_transform);
	}
}