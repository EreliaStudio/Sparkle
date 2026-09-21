#pragma once

#include <functional>

#include "engine/engine.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class RenderingEngine final : public EventDispatcher,
								  public UpdatableByDeviceTrait,
								  public RenderSnapshotContributorTrait
	{
	public:
		static inline const spk::RenderPass::Key SceneRenderPassKey{
			.name = "engine.Scene",
			.order = -100};
		static inline const spk::RenderPass::Key PreSceneRenderPassKey{
			.name = "engine.PreScene",
			.order = -200};

	private:
		Engine *_engine = nullptr;

		void _visitEntity(
			Entity &entity,
			const std::function<void(RenderingObjectTrait &)> &callback) const;
		void _visitObjects(
			const std::function<void(RenderingObjectTrait &)> &callback) const;

		[[nodiscard]] bool _isAcceptingEvent() const override;
		void _propagateEvent(
			const std::function<void(EventDispatcher *)> &callback) override;
		[[nodiscard]] bool _canUpdateByDevice() const override;
		void _afterUpdate(UpdateContext &context, DeviceContext &deviceContext) override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _afterBuildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		explicit RenderingEngine(Engine *engine = nullptr);

		void setEngine(Engine *engine) noexcept;
		[[nodiscard]] Engine *engine() noexcept;
		[[nodiscard]] const Engine *engine() const noexcept;

		using UpdatableByDeviceTrait::updateState;
		using RenderSnapshotContributorTrait::buildRenderSnapshot;
	};
}
