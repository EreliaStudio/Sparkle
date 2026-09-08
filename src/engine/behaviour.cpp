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

	bool Behaviour::_isAcceptingInteraction() const
	{
		return isEffectivelyActive();
	}

	void Behaviour::_onGeometryChange(const spk::Rect2D &)
	{
	}

	void Behaviour::_buildRenderSnapshot(spk::RenderSnapshot::Builder &)
	{
	}

	void Behaviour::_updateState(UpdateContext &)
	{
	}

	void Behaviour::handleGeometryChange(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_onGeometryChange(_geometry);
	}

	const spk::Rect2D &Behaviour::geometry() const noexcept
	{
		return _geometry;
	}

	void Behaviour::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (isEffectivelyActive())
		{
			_buildRenderSnapshot(builder);
		}
	}

	void Behaviour::updateState(UpdateContext &context)
	{
		if (!isEffectivelyActive())
		{
			return;
		}

		_updateState(context);
	}
}
