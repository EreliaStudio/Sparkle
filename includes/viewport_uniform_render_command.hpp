#pragma once

#include <cstddef>

#include "matrix.hpp"
#include "rect2d.hpp"
#include "render_command.hpp"

namespace spk
{
	class UniformBuffer;
	class ViewportUniformRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t MatrixUBOBindingPoint = 0;

	private:
		[[nodiscard]] static UniformBuffer &_sharedBuffer();

		Matrix4x4 _matrix;

	public:
		explicit ViewportUniformRenderCommand(const Rect2D &viewport);

		void execute(RenderContext &renderContext) const override;
	};
}
