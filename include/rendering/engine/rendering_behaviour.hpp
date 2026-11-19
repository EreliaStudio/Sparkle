#pragma once

#include "engine/behaviour.hpp"
#include "engine/behaviour2d.hpp"
#include "engine/behaviour3d.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderingBehaviour : public Behaviour,
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
		using Behaviour::Behaviour;
		using Behaviour::updateState;
		using EngineView::Contributor::updateState;
	};

	class RenderingBehaviour2D : public Behaviour2D,
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
		using Behaviour2D::Behaviour2D;
		using Behaviour2D::updateState;
		using EngineView::Contributor::updateState;
	};

	class RenderingBehaviour3D : public Behaviour3D,
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
		using Behaviour3D::Behaviour3D;
		using Behaviour3D::updateState;
		using EngineView::Contributor::updateState;
	};
}
