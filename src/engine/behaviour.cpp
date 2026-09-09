#include "engine/behaviour.hpp"

#include "core/context/update_context.hpp"
#include "engine/entity.hpp"
#include "rendering/render_snapshot.hpp"

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

	bool Behaviour::_isAcceptingEvent() const
	{
		return isEffectivelyActive();
	}

	bool Behaviour::_canUpdate() const
	{
		return isEffectivelyActive();
	}

	bool Behaviour::_canBuildRenderSnapshot() const
	{
		return isEffectivelyActive();
	}
}
