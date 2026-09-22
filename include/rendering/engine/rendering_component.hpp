#pragma once

#include "engine/component.hpp"
#include "engine/component2d.hpp"
#include "engine/component3d.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class RenderingComponent : public Component,
							   public RenderingObjectTrait
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

		[[nodiscard]] bool _canBuildRenderSnapshot() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Component::Component;
		using RenderingObjectTrait::updateState;
	};

	class RenderingComponent2D : public Component2D,
								 public RenderingObjectTrait
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

		[[nodiscard]] bool _canBuildRenderSnapshot() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Component2D::Component2D;
		using RenderingObjectTrait::updateState;
	};

	class RenderingComponent3D : public Component3D,
								 public RenderingObjectTrait
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

		[[nodiscard]] bool _canBuildRenderSnapshot() const override
		{
			return isEffectivelyActive();
		}

	public:
		using Component3D::Component3D;
		using RenderingObjectTrait::updateState;
	};
}
