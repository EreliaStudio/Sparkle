#pragma once

#include "engine/behaviour.hpp"
#include "engine/behaviour2d.hpp"
#include "engine/behaviour3d.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class RenderingBehaviour : public Behaviour,
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
		using Behaviour::Behaviour;
		using Behaviour::updateState;
		using RenderingObjectTrait::updateState;
	};

	class RenderingBehaviour2D : public Behaviour2D,
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
		using Behaviour2D::Behaviour2D;
		using Behaviour2D::updateState;
		using RenderingObjectTrait::updateState;
	};

	class RenderingBehaviour3D : public Behaviour3D,
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
		using Behaviour3D::Behaviour3D;
		using Behaviour3D::updateState;
		using RenderingObjectTrait::updateState;
	};
}
