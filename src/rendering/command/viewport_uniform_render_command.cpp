#include "rendering/command/viewport_uniform_render_command.hpp"

#include <stdexcept>

#include "core/context/render_context.hpp"
#include "graphics/opengl/uniform_buffer.hpp"

namespace spk
{

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
		static thread_local auto cpu = [] {
			Buffer::Builder builder;
			builder.resize<Matrix4x4>(1);
			return std::move(builder).buildMutable();
		}();
		static thread_local UniformBuffer buffer(MatrixUBOBindingPoint, sizeof(Matrix4x4));
		if (buffer.buffer() == nullptr)
		{
			buffer.bind(&cpu);
		}
		cpu.cast<Matrix4x4>()[0] = _matrix;
		cpu.validate();
		buffer.activate(context);
	}
}
