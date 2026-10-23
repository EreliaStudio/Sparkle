#include "engine/publishable_component.hpp"

namespace spk
{
	PublishableComponent::PublishableComponent(const std::string &name, Entity *owner) :
		PublishableAttachment<Component>(name, owner)
	{
	}

	PublishableComponent::PublishableComponent(Entity *owner) :
		PublishableComponent("Unnamed publishable component", owner)
	{
	}
}
