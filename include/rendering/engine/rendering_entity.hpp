#pragma once

#include "engine/entity.hpp"
#include "engine/entity2d.hpp"
#include "engine/entity3d.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class RenderingEntity : public Entity,
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
		using Entity::Entity;
		using Entity::updateState;
		using RenderingObjectTrait::updateState;
	};

	class RenderingEntity2D : public Entity2D,
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
		using Entity2D::Entity2D;
		using Entity2D::updateState;
		using RenderingObjectTrait::updateState;
	};

	class RenderingEntity3D : public Entity3D,
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
		using Entity3D::Entity3D;
		using Entity3D::updateState;
		using RenderingObjectTrait::updateState;
	};
}
