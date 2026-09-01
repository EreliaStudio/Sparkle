#include "engine/system_participant2d.hpp"

#include <stdexcept>

namespace spk
{
	System::Participant2D::Participant2D(const std::string &name, Entity2D *owner) :
		Participant(name)
	{
		attach(owner);
	}

	System::Participant2D::Participant2D(Entity2D *owner) :
		Participant2D("Unnamed participant", owner)
	{
	}

	void System::Participant2D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Participant::attach(nullptr);
			return;
		}

		Entity2D *owner2D = dynamic_cast<Entity2D *>(owner);

		if (owner2D == nullptr)
		{
			throw std::invalid_argument("A System::Participant2D can only be attached to an Entity2D");
		}

		Participant::attach(owner2D);
	}

	Entity2D *System::Participant2D::owner()
	{
		return static_cast<Entity2D *>(Participant::owner());
	}

	const Entity2D *System::Participant2D::owner() const
	{
		return static_cast<const Entity2D *>(Participant::owner());
	}
}
