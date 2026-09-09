#include "engine/component3d.hpp"

#include <stdexcept>

namespace spk
{
	Component3D::Component3D(const std::string &name, Entity3D *owner) :
		Component(name)
	{
		attach(owner);
	}

	Component3D::Component3D(Entity3D *owner) :
		Component3D("Unnamed component", owner)
	{
	}

	void Component3D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Component::attach(nullptr);
			return;
		}

		Entity3D *owner3D = dynamic_cast<Entity3D *>(owner);

		if (owner3D == nullptr)
		{
			throw std::invalid_argument("A Component3D can only be attached to an Entity3D");
		}

		Component::attach(owner3D);
	}

	Entity3D *Component3D::owner()
	{
		return static_cast<Entity3D *>(Component::owner());
	}

	const Entity3D *Component3D::owner() const
	{
		return static_cast<const Entity3D *>(Component::owner());
	}
}
