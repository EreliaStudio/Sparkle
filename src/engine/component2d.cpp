#include "engine/component2d.hpp"

#include <stdexcept>

namespace spk
{
	Component2D::Component2D(const std::string &name, Entity2D *owner) :
		Component(name)
	{
		attach(owner);
	}

	Component2D::Component2D(Entity2D *owner) :
		Component2D("Unnamed component", owner)
	{
	}

	void Component2D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Component::attach(nullptr);
			return;
		}

		Entity2D *owner2D = dynamic_cast<Entity2D *>(owner);

		if (owner2D == nullptr)
		{
			throw std::invalid_argument("A Component2D can only be attached to an Entity2D");
		}

		Component::attach(owner2D);
	}

	Entity2D *Component2D::owner()
	{
		return static_cast<Entity2D *>(Component::owner());
	}

	const Entity2D *Component2D::owner() const
	{
		return static_cast<const Entity2D *>(Component::owner());
	}
}
