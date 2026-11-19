#pragma once

#include "engine/component.hpp"
#include "engine/component2d.hpp"
#include "engine/component3d.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderingComponent : public Component,
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
		using Component::Component;
		using EngineView::Contributor::updateState;
	};

	class RenderingComponent2D : public Component2D,
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
		using Component2D::Component2D;
		using EngineView::Contributor::updateState;
	};

	class RenderingComponent3D : public Component3D,
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
		using Component3D::Component3D;
		using EngineView::Contributor::updateState;
	};
}
