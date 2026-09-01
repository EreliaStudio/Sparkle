#include "engine/entity3d.hpp"

#include "engine/transform3d.hpp"

namespace spk
{
	Entity3D::Entity3D(const std::string &name, Entity *parent) :
		Entity(name, parent),
		_transform(addParticipant<Transform3D>("Transform"))
	{
		_onParentEditionContract = subscribeToParentEdition(
			[this](const Entity *) {
				_transform._clearWorldCaches();
				_transform._clearDescendantWorldCaches();
			});
	}

	Transform3D &Entity3D::transform()
	{
		return _transform;
	}

	const Transform3D &Entity3D::transform() const
	{
		return _transform;
	}
}
