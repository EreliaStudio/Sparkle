#include "scissor_render_command.hpp"

#include <Windows.h>
#include <gl/gl.h>

#include "render_context.hpp"

namespace spk
{
	ScissorRenderCommand::ScissorRenderCommand(const Rect2D &scissor) :
		_scissor(scissor)
	{
	}

	void ScissorRenderCommand::execute(RenderContext &renderContext) const
	{
		const GLint y = static_cast<GLint>(renderContext.targetSurface->geometry().size.y) - _scissor.y - static_cast<GLint>(_scissor.height);

		::glEnable(GL_SCISSOR_TEST);
		::glScissor(
			static_cast<GLint>(_scissor.x),
			y,
			static_cast<GLsizei>(_scissor.width),
			static_cast<GLsizei>(_scissor.height));
	}
}