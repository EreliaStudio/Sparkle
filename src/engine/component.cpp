#include "engine/component.hpp"

#include "rendering/render_snapshot.hpp"

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

	bool Component::_canBuildRenderSnapshot() const
	{
		return isEffectivelyActive();
	}
}
