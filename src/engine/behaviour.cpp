#include "engine/behaviour.hpp"

#include "core/context/update_context.hpp"
#include "engine/entity.hpp"

namespace spk
{
	Behaviour::Behaviour(const std::string &name, Entity *owner) :
		EntityAttachment(name, owner)
	{
	}

	Behaviour::Behaviour(Entity *owner) :
		Behaviour("Unnamed behaviour", owner)
	{
	}

	bool Behaviour::_canUpdate() const
	{
		return isEffectivelyActive();
	}
}
