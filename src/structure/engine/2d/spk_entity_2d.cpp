#include "structure/engine/2d/spk_entity_2d.hpp"

namespace spk
{
	Entity2D::Entity2D(const std::wstring &p_name, const spk::SafePointer<GenericEntity> &p_owner):
		spk::GenericEntity(p_name),
		_transform(addComponent<Transform2D>())
	{
		_ownerTransformEditionContract = _transform->addOnEditionCallback(
			[&]()
			{
				for (auto &child : children())
				{
					child.upCast<spk::Entity2D>()->transform()._updateModel();
				}
			});
	}

	Entity2D::Entity2D() :
		Entity2D(L"Unnamed entity")
	{

	}

	const spk::Vector2 &Entity2D::position() const
	{
		return (_transform->position());
	}
	
	Transform2D &Entity2D::transform()
	{
		return (*_transform);
	}
	
	const Transform2D &Entity2D::transform() const
	{
		return (*_transform);
	}
}