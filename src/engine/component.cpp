#include "engine/component.hpp"

namespace spk
{
	Component::Component(const std::string &name, Entity *owner) :
		EntityAttachment(name, owner)
	{
	}

	Component::Component(Entity *owner) :
		Component("Unnamed component", owner)
	{
	}
}
