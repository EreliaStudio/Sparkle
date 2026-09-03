#include "engine/behaviour3d.hpp"

#include <stdexcept>

namespace spk
{
	Behaviour3D::Behaviour3D(const std::string &name, Entity3D *owner) :
		Behaviour(name)
	{
		attach(owner);
	}

	Behaviour3D::Behaviour3D(Entity3D *owner) :
		Behaviour3D("Unnamed participant", owner)
	{
	}

	void Behaviour3D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Behaviour::attach(nullptr);
			return;
		}

		Entity3D *owner2D = dynamic_cast<Entity3D *>(owner);

		if (owner2D == nullptr)
		{
			throw std::invalid_argument("A Behaviour3D can only be attached to an Entity3D");
		}

		Behaviour::attach(owner2D);
	}

	Entity3D *Behaviour3D::owner()
	{
		return static_cast<Entity3D *>(Behaviour::owner());
	}

	const Entity3D *Behaviour3D::owner() const
	{
		return static_cast<const Entity3D *>(Behaviour::owner());
	}
}
