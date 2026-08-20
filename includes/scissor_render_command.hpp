#pragma once

#include "rect2d.hpp"
#include "render_command.hpp"

namespace spk
{
	class ScissorRenderCommand final : public RenderCommand
	{
	private:
		Rect2D _scissor;

	public:
		explicit ScissorRenderCommand(const Rect2D &scissor);
		void execute(RenderContext &renderContext) const override;
	};
}