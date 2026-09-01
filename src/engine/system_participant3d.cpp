#include "engine/system_participant3d.hpp"

#include <stdexcept>

namespace spk
{
	System::Participant3D::Participant3D(const std::string &name, Entity3D *owner) :
		Participant(name)
	{
		attach(owner);
	}

	System::Participant3D::Participant3D(Entity3D *owner) :
		Participant3D("Unnamed participant", owner)
	{
	}

	void System::Participant3D::attach(Entity *owner)
	{
		if (owner == nullptr)
		{
			Participant::attach(nullptr);
			return;
		}

		Entity3D *owner3D = dynamic_cast<Entity3D *>(owner);

		if (owner3D == nullptr)
		{
			throw std::invalid_argument("A System::Participant3D can only be attached to an Entity3D");
		}

		Participant::attach(owner3D);
	}

	Entity3D *System::Participant3D::owner()
	{
		return static_cast<Entity3D *>(Participant::owner());
	}

	const Entity3D *System::Participant3D::owner() const
	{
		return static_cast<const Entity3D *>(Participant::owner());
	}
}
