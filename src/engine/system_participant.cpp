#include "engine/system_participant.hpp"

#include "rendering/render_snapshot.hpp"

namespace spk
{
	System::Participant::Participant(const std::string &name, Entity *owner) :
		EntityAttachment(name, owner)
	{
	}

	System::Participant::Participant(Entity *owner) :
		Participant("Unnamed participant", owner)
	{
	}

	bool System::Participant::_canBuildRenderSnapshot() const
	{
		return isEffectivelyActive();
	}

	void System::Participant::_onGeometryChange(const spk::Rect2D &)
	{
	}
}
