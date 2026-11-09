#include "rendering/command/viewport_render_command.hpp"

#include <GL/glew.h>

#include "core/context/render_context.hpp"

namespace spk
{
	ViewportRenderCommand::ViewportRenderCommand(const Rect2D &viewport) :
		_viewport(viewport)
	{
	}

	void ViewportRenderCommand::execute(RenderContext &renderContext) const
	{
		const GLint y = static_cast<GLint>(renderContext.renderTargetSize().y) - _viewport.y - static_cast<GLint>(_viewport.height);

		::glViewport(
			static_cast<GLint>(_viewport.x),
			y,
			static_cast<GLsizei>(_viewport.width),
			static_cast<GLsizei>(_viewport.height));
	}
}