#pragma once

#include <cstddef>

#include "math/matrix.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class UniformBuffer;
	class ViewportUniformRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t MatrixUBOBindingPoint = 0;

	private:
		Matrix4x4 _matrix;

	public:
		explicit ViewportUniformRenderCommand(const Rect2D &viewport);

		void execute(RenderContext &renderContext) const override;
	};
}
