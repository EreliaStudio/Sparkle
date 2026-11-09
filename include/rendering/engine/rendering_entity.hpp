#pragma once

#include "engine/entity.hpp"
#include "engine/entity2d.hpp"
#include "engine/entity3d.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderingEntity : public Entity,
							public EngineView::Contributor
	{
	protected:
		[[nodiscard]] bool _isAcceptingEvent() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canUpdateByDevice() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canContribute() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Entity::Entity;
		using Entity::updateState;
		using EngineView::Contributor::updateState;
	};

	class RenderingEntity2D : public Entity2D,
							  public EngineView::Contributor
	{
	protected:
		[[nodiscard]] bool _isAcceptingEvent() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canUpdateByDevice() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canContribute() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Entity2D::Entity2D;
		using Entity2D::updateState;
		using EngineView::Contributor::updateState;
	};

	class RenderingEntity3D : public Entity3D,
							  public EngineView::Contributor
	{
	protected:
		[[nodiscard]] bool _isAcceptingEvent() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canUpdateByDevice() const override
		{
			return isEffectivelyActive();
		}

		[[nodiscard]] bool _canContribute() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Entity3D::Entity3D;
		using Entity3D::updateState;
		using EngineView::Contributor::updateState;
	};
}
