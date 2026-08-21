#include "clear_render_command.hpp"

#include <Windows.h>
#include <gl/gl.h>

namespace spk
{
	ClearRenderCommand::ClearRenderCommand(Color color, Mask mask) :
		_color(color),
		_mask(mask)
	{
	}

	void ClearRenderCommand::execute(RenderContext &) const
	{
		GLbitfield mask = 0;

		if ((_mask & Mask::Color) != Mask::None)
		{
			mask |= GL_COLOR_BUFFER_BIT;
		}

		if ((_mask & Mask::Depth) != Mask::None)
		{
			mask |= GL_DEPTH_BUFFER_BIT;
		}

		if ((_mask & Mask::Stencil) != Mask::None)
		{
			mask |= GL_STENCIL_BUFFER_BIT;
		}

		if ((_mask & Mask::Color) != Mask::None)
		{
			::glClearColor(_color.r, _color.g, _color.b, _color.a);
		}

		if (mask != 0)
		{
			::glClear(mask);
		}
	}
}