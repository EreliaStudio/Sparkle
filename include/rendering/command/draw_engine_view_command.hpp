#pragma once

#include <memory>

#include "math/rect2d.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class DrawEngineViewCommand final : public RenderCommand
	{
	private:
		std::shared_ptr<EngineView::Output> _output;
		Rect2D _rectangle;

	public:
		DrawEngineViewCommand(std::shared_ptr<EngineView::Output> output, Rect2D rectangle);
		void execute(RenderContext &context) const override;
	};
}
