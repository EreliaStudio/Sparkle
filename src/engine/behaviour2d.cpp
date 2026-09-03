#include "engine/behaviour2d.hpp"

#include <stdexcept>

namespace spk
{
	Behaviour2D::Behaviour2D(const std::string &name, Entity2D *owner) :
		Behaviour(name)
	{
		attach(owner);
	}

	Behaviour2D::Behaviour2D(Entity2D *owner) :
		Behaviour2D("Unnamed participant", owner)
	{
	}

	void Behaviour2D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Behaviour::attach(nullptr);
			return;
		}

		Entity2D *owner2D = dynamic_cast<Entity2D *>(owner);

		if (owner2D == nullptr)
		{
			throw std::invalid_argument("A Behaviour2D can only be attached to an Entity2D");
		}

		Behaviour::attach(owner2D);
	}

	Entity2D *Behaviour2D::owner()
	{
		return static_cast<Entity2D *>(Behaviour::owner());
	}

	const Entity2D *Behaviour2D::owner() const
	{
		return static_cast<const Entity2D *>(Behaviour::owner());
	}
}
