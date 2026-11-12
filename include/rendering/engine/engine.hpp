#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

#include "engine/transform3d.hpp"
#include "light/contributor.hpp"

#include "container/thread_safe_slot.hpp"
#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"
#include "engine/engine_headless.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderCommand;

	class Engine final : public EngineHeadless, public EventDispatcher, public UpdatableByDeviceTrait, public RenderSnapshotContributorTrait
	{
	public:
		static inline const RenderPass::Key SceneRenderPassKey{"engine.Scene", -100};
		static inline const RenderPass::Key PreSceneRenderPassKey{"engine.PreScene", -200};

	private:
		friend class Light::Channel;

		struct ViewCapture
		{
			std::uint64_t identity;
			std::optional<EngineView::Order> order;
			EngineView::CaptureContext context;
		};

		struct LightContracts
		{
			Light::Contributor::OnEditionContract edition;
			Light::Contributor::ActivationContract activation;
			Light::Contributor::DeactivationContract deactivation;
		};
		struct EntityContracts
		{
			Entity::ActivationContract activation;
			Entity::DeactivationContract deactivation;
			Entity::OnParentEditionContract parent;
		};
		mutable std::mutex _viewMutex;
		Registry<Light::Contributor, EngineHeadless *>::OnEditionContract _lightAddition;
		Registry<Light::Contributor, EngineHeadless *>::OnEditionContract _lightRemoval;
		Registry<Transform3D, EngineHeadless *>::OnEditionContract _transformAddition;
		Registry<Transform3D, EngineHeadless *>::OnEditionContract _transformRemoval;
		Registry<Entity, EngineHeadless *>::OnEditionContract _entityAddition;
		Registry<Entity, EngineHeadless *>::OnEditionContract _entityRemoval;
		std::unordered_map<Light::Contributor *, LightContracts> _lightContracts;
		std::unordered_map<Transform3D *, Transform3D::OnEditionContract> _transformContracts;
		std::unordered_map<Entity *, EntityContracts> _entityContracts;
		std::vector<std::unique_ptr<EngineView>> _views;
		std::uint64_t _nextViewIdentity = 1;
		std::uint64_t _nextCaptureOrdinal = 0;
		ThreadSafeSlot<RenderSnapshot> _renderSnapshot;

		void _trackLight(Light::Contributor &light);
		void _trackTransform(Transform3D &transform);
		void _trackEntity(Entity &entity);
		void _invalidateAllLightChannels();
		void _onEntityChanged(const Entity &entity);
		void _onTransformChanged(const Transform3D &transform);
		[[nodiscard]] static bool _isDescendantOf(const Entity *candidate, const Entity *ancestor) noexcept;
		void _visitEntity(Entity &entity, const std::function<void(EngineView::Contributor &)> &callback) const;
		void _visitObjects(const std::function<void(EngineView::Contributor &)> &callback);
		[[nodiscard]] EngineView *_findView(const std::string &name) const noexcept;
		[[nodiscard]] std::vector<ViewCapture> _captureViews();
		[[nodiscard]] std::vector<Light::Selection::Entry> _captureLightCandidates(const Camera3D &camera) const;
		[[nodiscard]] std::unique_ptr<const RenderCommand> _buildViewCommand(const EngineView::CaptureContext &context);
		[[nodiscard]] bool _commitSnapshot(const std::vector<ViewCapture> &views, RenderSnapshot snapshot);
		static void _validateViewPasses(const std::string &name, const RenderSnapshot::Builder &builder);

		[[nodiscard]] bool _isAcceptingEvent() const override;
		void _propagateEvent(const std::function<void(EventDispatcher *)> &callback) override;
		[[nodiscard]] bool _canUpdateByDevice() const override;
		void _afterUpdate(UpdateContext &context, DeviceContext &deviceContext) override;
		[[nodiscard]] bool _canBuildRenderSnapshot() const override;
		void _afterBuildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		Engine();
		~Engine() override;

		EngineView &addView(std::string name, CameraComponent3D *camera, Vector2UInt resolution, std::optional<EngineView::Order> order = std::nullopt);
		[[nodiscard]] EngineView &view(const std::string &name);
		[[nodiscard]] const EngineView &view(const std::string &name) const;
		[[nodiscard]] std::shared_ptr<EngineView::Output> viewOutput(const std::string &name) const;
		void removeView(const std::string &name);
		void setViewOrder(EngineView &view, std::optional<EngineView::Order> order);
		void refreshRenderSnapshot();
		[[nodiscard]] std::shared_ptr<const RenderSnapshot> latestRenderSnapshot() const noexcept;

		using EngineHeadless::updateState;
		using RenderSnapshotContributorTrait::buildRenderSnapshot;
		using UpdatableByDeviceTrait::updateState;
	};
}
