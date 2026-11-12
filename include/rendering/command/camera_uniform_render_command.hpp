#pragma once

#include <cstddef>
#include <type_traits>

#include "graphics/opengl/uniform_buffer.hpp"
#include "math/matrix.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	struct alignas(16) CameraData
	{
		Matrix4x4 projection;
		Matrix4x4 view;
		Matrix4x4 inverseView;
		Matrix4x4 viewProjection;
	};

	static_assert(sizeof(CameraData) == 4 * sizeof(Matrix4x4));
	static_assert(alignof(CameraData) == 16);
	static_assert(std::is_standard_layout_v<CameraData>);
	static_assert(std::is_trivially_copyable_v<CameraData>);
	static_assert(offsetof(CameraData, projection) == 0);
	static_assert(offsetof(CameraData, view) == 64);
	static_assert(offsetof(CameraData, inverseView) == 128);
	static_assert(offsetof(CameraData, viewProjection) == 192);

	class CameraUniformRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t BindingPoint = 1;

	private:
		UniformBuffer _buffer;

	public:
		explicit CameraUniformRenderCommand(CameraData data);

		void execute(RenderContext &context) const override;
	};
}
