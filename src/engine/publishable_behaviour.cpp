#include "engine/publishable_behaviour.hpp"

namespace spk
{
	PublishableBehaviour::PublishableBehaviour(const std::string &name, Entity *owner) :
		PublishableAttachment<Behaviour>(name, owner)
	{
	}

	PublishableBehaviour::PublishableBehaviour(Entity *owner) :
		PublishableBehaviour("Unnamed publishable behaviour", owner)
	{
	}
}
