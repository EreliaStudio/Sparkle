#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/updatable_by_device_trait.hpp"
#include "graphics/color.hpp"
#include "graphics/opengl/texture.hpp"
#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	class Engine;

	class EngineView final
	{
	public:
		using Layers = std::vector<std::string>;
		using Order = RenderPass::Order;

		enum class DetachedOutputPolicy
		{
			Invalid,
			KeepLastFrame,
			DefaultTexture
		};

		struct CaptureContext
		{
			std::string name;
			std::uint64_t identity = 0;
			Vector2UInt resolution{};
			Matrix4x4 projectionMatrix{};
			Matrix4x4 viewMatrix{};
			Matrix4x4 inverseViewMatrix{};
			Matrix4x4 viewProjectionMatrix{};
			std::optional<Layers> layers;
			RGB backgroundColor{};
			float exposure = 1.0f;
			float targetFPS = 60.0f;
			DetachedOutputPolicy detachedOutputPolicy = DetachedOutputPolicy::Invalid;
			Texture::Handle fallbackTexture;

			[[nodiscard]] bool isLayerCompatible(const Layers &candidate) const;
		};

		class Contributor : public EventDispatcher, public UpdatableByDeviceTrait
		{
		private:
			Layers _layers;

		protected:
			[[nodiscard]] bool _isAcceptingEvent() const override
			{
				return true;
			}
			void _propagateEvent(const std::function<void(EventDispatcher *)> &) override
			{
			}
			[[nodiscard]] virtual bool _canContribute() const
			{
				return true;
			}

		public:
			virtual ~Contributor() = default;

			void setLayers(Layers layers);
			[[nodiscard]] const Layers &layers() const noexcept;
			[[nodiscard]] bool canContribute() const;
			virtual void buildViewRenderSnapshot(RenderSnapshot::Builder &, const CaptureContext &)
			{
			}
		};

		static inline const RenderPass::Key ShadowPassKey{"sparkle.engineview.shadow", -1000000};
		static inline const RenderPass::Key SkyboxPassKey{"sparkle.engineview.skybox", -1};
		static inline const RenderPass::Key OpaquePassKey{"sparkle.engineview.opaque", 0};
		static inline const RenderPass::Key WBOITPassKey{"sparkle.engineview.wboit", 1000000};
		static inline const RenderPass::Key CompositionPassKey{"sparkle.engineview.composition", 2000000};

	private:
		friend class Engine;

		Engine *_engine;
		const std::string _name;
		const std::uint64_t _identity;
		CameraComponent3D *_camera;
		Vector2UInt _resolution;
		std::optional<Order> _order;
		std::optional<Layers> _layers;
		DetachedOutputPolicy _detachedOutputPolicy = DetachedOutputPolicy::Invalid;
		Texture::Handle _fallbackTexture;
		RGB _backgroundColor{};
		float _exposure = 1.0f;
		float _targetFPS = 60.0f;

		EngineView(Engine &engine, std::string name, std::uint64_t identity,
				   CameraComponent3D *camera, Vector2UInt resolution, std::optional<Order> order);
		[[nodiscard]] CaptureContext _capture() const;

	public:
		EngineView(const EngineView &) = delete;
		EngineView &operator=(const EngineView &) = delete;
		EngineView(EngineView &&) = delete;
		EngineView &operator=(EngineView &&) = delete;

		[[nodiscard]] const std::string &name() const noexcept;
		[[nodiscard]] std::uint64_t identity() const noexcept;
		[[nodiscard]] CameraComponent3D *camera() const noexcept;
		void setCamera(CameraComponent3D *camera) noexcept;
		void detachCamera() noexcept;
		[[nodiscard]] Vector2UInt resolution() const noexcept;
		void setResolution(Vector2UInt resolution);
		[[nodiscard]] std::optional<Order> order() const noexcept;
		void setOrder(std::optional<Order> order);
		void setLayers(Layers layers);
		void clearLayerFilter() noexcept;
		[[nodiscard]] const std::optional<Layers> &layers() const noexcept;
		[[nodiscard]] DetachedOutputPolicy detachedOutputPolicy() const noexcept;
		void setDetachedOutputPolicy(DetachedOutputPolicy policy);
		void setFallbackTexture(Texture::Handle texture) noexcept;
		[[nodiscard]] const Texture::Handle &fallbackTexture() const noexcept;
		void setBackgroundColor(RGB color);
		[[nodiscard]] RGB backgroundColor() const noexcept;
		void setExposure(float exposure);
		[[nodiscard]] float exposure() const noexcept;
		void setTargetFPS(float fps);
		[[nodiscard]] float targetFPS() const noexcept;
	};
}
