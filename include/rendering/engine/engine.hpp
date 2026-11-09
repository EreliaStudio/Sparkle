#pragma once

#include <functional>

#include "engine/engine_headless.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class Engine final : public EngineHeadless, public EventDispatcher, public UpdatableByDeviceTrait, public RenderSnapshotContributorTrait
	{
	public:
		static inline const spk::RenderPass::Key SceneRenderPassKey{
			.name = "engine.Scene",
			.order = -100};
		static inline const spk::RenderPass::Key PreSceneRenderPassKey{
			.name = "engine.PreScene",
			.order = -200};

	private:
		void _visitEntity(
			Entity &entity,
			const std::function<void(RenderingObjectTrait &)> &callback) const;
		void _visitObjects(
			const std::function<void(RenderingObjectTrait &)> &callback);

		[[nodiscard]] bool _isAcceptingEvent() const override;
		void _propagateEvent(
			const std::function<void(EventDispatcher *)> &callback) override;
		[[nodiscard]] bool _canUpdateByDevice() const override;
		void _afterUpdate(UpdateContext &context, DeviceContext &deviceContext) override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _afterBuildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		Engine() = default;

		using EngineHeadless::updateState;
		using RenderSnapshotContributorTrait::buildRenderSnapshot;
		using UpdatableByDeviceTrait::updateState;
	};
}
