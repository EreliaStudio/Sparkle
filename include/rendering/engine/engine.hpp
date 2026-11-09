#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "container/thread_safe_slot.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "engine/engine_headless.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class Engine final : public EngineHeadless, public EventDispatcher, public UpdatableByDeviceTrait, public RenderSnapshotContributorTrait
	{
	public:
		static inline const RenderPass::Key SceneRenderPassKey{"engine.Scene", -100};
		static inline const RenderPass::Key PreSceneRenderPassKey{"engine.PreScene", -200};

	private:
		struct ViewCapture
		{
			std::uint64_t identity;
			std::optional<EngineView::Order> order;
			EngineView::CaptureContext context;
		};

		mutable std::mutex _viewMutex;
		std::vector<std::unique_ptr<EngineView>> _views;
		std::uint64_t _nextViewIdentity = 1;
		ThreadSafeSlot<RenderSnapshot> _renderSnapshot;

		void _visitEntity(Entity &entity, const std::function<void(EngineView::Contributor &)> &callback) const;
		void _visitObjects(const std::function<void(EngineView::Contributor &)> &callback);
		[[nodiscard]] EngineView *_findView(const std::string &name) const noexcept;
		[[nodiscard]] std::vector<ViewCapture> _captureViews() const;
		[[nodiscard]] RenderSnapshot _buildViewSnapshot(const EngineView::CaptureContext &context);
		[[nodiscard]] bool _commitSnapshot(const std::vector<ViewCapture> &views, RenderSnapshot snapshot);
		static void _validateViewPasses(const std::string &name, const RenderSnapshot::Builder &builder);

		[[nodiscard]] bool _isAcceptingEvent() const override;
		void _propagateEvent(const std::function<void(EventDispatcher *)> &callback) override;
		[[nodiscard]] bool _canUpdateByDevice() const override;
		void _afterUpdate(UpdateContext &context, DeviceContext &deviceContext) override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _afterBuildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		Engine() = default;

		EngineView &addView(std::string name, CameraComponent3D *camera, Vector2UInt resolution, std::optional<EngineView::Order> order = std::nullopt);
		[[nodiscard]] EngineView &view(const std::string &name);
		[[nodiscard]] const EngineView &view(const std::string &name) const;
		void removeView(const std::string &name);
		void setViewOrder(EngineView &view, std::optional<EngineView::Order> order);
		void refreshRenderSnapshot();
		[[nodiscard]] std::shared_ptr<const RenderSnapshot> latestRenderSnapshot() const noexcept;

		using EngineHeadless::updateState;
		using RenderSnapshotContributorTrait::buildRenderSnapshot;
		using UpdatableByDeviceTrait::updateState;
	};
}
