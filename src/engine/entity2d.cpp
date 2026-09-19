#include "engine/entity2d.hpp"

#include "engine/transform2d.hpp"

namespace spk
{
	Entity2D::Entity2D(const std::string &name, Entity *parent) :
		Entity(name, parent),
		_transform(addComponent<Transform2D>("Transform"))
	{
		_onParentEditionContract = subscribeToParentEdition(
			[this](const Entity *) {
				_transform._clearWorldCaches();
				_transform._clearDescendantWorldCaches();
			});
	}

	Transform2D &Entity2D::transform()
	{
		return _transform;
	}

	const Transform2D &Entity2D::transform() const
	{
		return _transform;
	}
}
