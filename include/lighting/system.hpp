#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "graphics/color.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "light/selection.hpp"
#include "math/vector3.hpp"

namespace spk
{
	class RenderContext;
}

namespace spk::Lighting
{
	// A captured Forward-lighting payload, shared by every opaque draw of one view.
	// Its GPU instances are automatically owned by each executing Window::Surface.
	class System final
	{
	public:
		static constexpr std::size_t BindingPoint = 3;

		struct alignas(16) Header
		{
			RGB ambient;
			float padding = 0.0f;
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
			std::uint32_t padding[3]{};
		};

		static_assert(sizeof(Header) == 32 && alignof(Header) == 16);
		static_assert(offsetof(Header, ambient) == 0 && offsetof(Header, count) == 16);
		static_assert(sizeof(LightData) == 64 && alignof(LightData) == 16);
		static_assert(offsetof(LightData, position) == 0);
		static_assert(offsetof(LightData, direction) == 16);
		static_assert(offsetof(LightData, color) == 32);
		static_assert(offsetof(LightData, type) == 48);
		static_assert(std::is_standard_layout_v<LightData> && std::is_trivially_copyable_v<LightData>);

	private:
		ShaderStorageBuffer _lights;

	public:
		System(const Light::Selection &selection, RGB ambient);

		[[nodiscard]] const ShaderStorageBuffer &buffer() const noexcept;
		void activate(RenderContext &context) const;
	};
}
