#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "graphics/color.hpp"
#include "graphics/opengl/padding.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "light/selection.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"

namespace spk
{
	class RenderContext;
}

namespace spk::Light
{
	class Channel;
}

namespace spk::Lighting
{
	// A captured Forward-lighting payload, shared by every opaque draw of one view.
	// Its GPU instances are automatically owned by each executing Window::Surface.
	class System final
	{
	public:
		static constexpr std::size_t BindingPoint = 3;
		static constexpr std::size_t ShadowBindingPoint = 4;
		static constexpr std::size_t MaxShadowSamplers = 16;
		static constexpr std::size_t MaxPointShadowSamplers = 8;
		static constexpr std::size_t PointShadowBindingBase = 1 + MaxShadowSamplers;

		struct alignas(16) Header
		{
			RGB ambient;
			SPK_PADDING(1);
			std::uint32_t count = 0;
			std::uint32_t localCount = 0;
			std::uint32_t directionalCount = 0;
			std::uint32_t reserved = 0;
		};

		struct alignas(16) LightData
		{
			Vector3 position;
			float range = 0.0f;
			Vector3 direction;
			float coneCosine = 1.0f;
			RGB color;
			float intensity = 0.0f;
			std::uint32_t type = 0;
			SPK_PADDING(3);
		};

		static_assert(sizeof(Header) == 32 && alignof(Header) == 16);
		static_assert(offsetof(Header, ambient) == 0 && offsetof(Header, count) == 16);
		static_assert(sizeof(LightData) == 64 && alignof(LightData) == 16);
		static_assert(offsetof(LightData, position) == 0);
		static_assert(offsetof(LightData, direction) == 16);
		static_assert(offsetof(LightData, color) == 32);
		static_assert(offsetof(LightData, type) == 48);
		static_assert(std::is_standard_layout_v<LightData> && std::is_trivially_copyable_v<LightData>);

		struct alignas(16) ShadowData
		{
			std::array<Matrix4x4, 6> viewProjection{};
			std::array<float, 4> parameters{};
			std::array<float, 4> pointPositionNear{};
			std::array<float, 4> pointFarResolution{};
			std::array<float, 4> cascadeSplits{};
			std::array<float, 4> cascadeDepthBias{};
			std::array<float, 4> cascadeNormalBias{};
			std::array<float, 4> cascadeDynamicSlots{};
		};

		static_assert(sizeof(ShadowData) == 496 && alignof(ShadowData) == 16);

		struct ShadowBindings
		{
			std::shared_ptr<const Buffer::Immutable> shadowData;
			std::shared_ptr<const Buffer::Immutable> historicalData;
			ShaderStorageBuffer buffer{ShadowBindingPoint, 0, sizeof(ShadowData)};
			std::vector<Sampler> samplers;
			std::unique_ptr<ShaderStorageBuffer> historicalLights;
		};

	private:
		std::shared_ptr<const Buffer::Immutable> _lightBuffer;
		ShaderStorageBuffer _lights;
		Header _header;
		std::vector<LightData> _lightData;
		Light::Selection _selection;
		std::shared_ptr<Light::Channel> _shadowChannel;
		std::uint64_t _captureOrdinal = 0;

	public:
		System(
			const Light::Selection &selection,
			RGB ambient,
			std::shared_ptr<Light::Channel> shadowChannel = {},
			std::uint64_t captureOrdinal = 0);

		[[nodiscard]] const ShaderStorageBuffer &buffer() const noexcept;
		void activate(RenderContext &context) const;
		[[nodiscard]] std::unique_ptr<ShadowBindings> activateShadows(RenderContext &context) const;
	};
}
