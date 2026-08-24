#pragma once

#include "math/rect2d.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class ViewportRenderCommand final : public RenderCommand
	{
	private:
		Rect2D _viewport;

	public:
		explicit ViewportRenderCommand(const Rect2D &viewport);
		void execute(RenderContext &renderContext) const override;
	};
}