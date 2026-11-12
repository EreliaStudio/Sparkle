#pragma once

#include <cstddef>
#include <type_traits>

#include "graphics/opengl/uniform_buffer.hpp"
#include "math/matrix.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class Camera3D;
	class Transform3D;

	class CameraUniformRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t BindingPoint = 1;

	private:
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

		UniformBuffer _buffer;

		[[nodiscard]] static CameraData _capture(const Camera3D &camera);
		[[nodiscard]] static CameraData _capture(const Camera3D &camera, const Transform3D &transform);

		explicit CameraUniformRenderCommand(CameraData data);

	public:
		explicit CameraUniformRenderCommand(const Camera3D &camera);
		CameraUniformRenderCommand(const Camera3D &camera, const Transform3D &transform);

		void execute(RenderContext &context) const override;
	};
}
