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

	void System::Participant::_onGeometryChange(const spk::Rect2D &)
	{
	}

	void System::Participant::_buildRenderSnapshot(spk::RenderSnapshot::Builder &)
	{
	}

	void System::Participant::handleGeometryChange(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_onGeometryChange(_geometry);
	}

	const spk::Rect2D &System::Participant::geometry() const noexcept
	{
		return _geometry;
	}

	void System::Participant::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (isActive())
		{
			_buildRenderSnapshot(builder);
		}
	}
}
