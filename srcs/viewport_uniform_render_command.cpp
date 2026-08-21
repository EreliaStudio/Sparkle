#include "viewport_uniform_render_command.hpp"

#include <stdexcept>

#include "render_context.hpp"
#include "uniform_buffer.hpp"

namespace spk
{
	UniformBuffer &ViewportUniformRenderCommand::_sharedBuffer()
	{
		static UniformBuffer buffer(MatrixUBOBindingPoint, sizeof(Matrix4x4));
		return buffer;
	}

	ViewportUniformRenderCommand::ViewportUniformRenderCommand(const Rect2D &viewport)
	{
		if (viewport.width == 0 || viewport.height == 0)
		{
			throw std::invalid_argument("ViewportUniformRenderCommand viewport cannot be empty");
		}
		_matrix = Matrix4x4::ortho(0.0f, static_cast<float>(viewport.width), static_cast<float>(viewport.height), 0.0f);
	}

	void ViewportUniformRenderCommand::execute(RenderContext &context) const
	{
		auto &buffer = _sharedBuffer();
		buffer.setData(_matrix);
		buffer.validate();
		buffer.activate(context);
	}
}
