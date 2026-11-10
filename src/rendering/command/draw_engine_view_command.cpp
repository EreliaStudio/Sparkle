#include "rendering/command/draw_engine_view_command.hpp"

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"
#include "rendering/command/image_render_command.hpp"

namespace spk
{
	DrawEngineViewCommand::DrawEngineViewCommand(
		std::shared_ptr<EngineView::Output> output,
		Rect2D rectangle) :
		_output(std::move(output)),
		_rectangle(rectangle)
	{
		if (!_output)
		{
			throw std::invalid_argument("DrawEngineViewCommand requires an output");
		}
	}

	void DrawEngineViewCommand::execute(RenderContext &context) const
	{
		if (context.targetSurface == nullptr)
		{
			return;
		}
		const auto texture = _output->latest(*context.targetSurface);
		if (texture)
		{
			ImageRenderCommand(texture, Texture::Section({0.0f, 1.0f}, {1.0f, -1.0f}), _rectangle).execute(context);
		}
	}
}
