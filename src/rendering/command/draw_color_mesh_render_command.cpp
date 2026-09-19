#include "rendering/command/draw_color_mesh_render_command.hpp"

#include <memory>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawColorMeshRenderCommand::_sharedProgram()
	{
		static auto program = []() {
			auto result = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_color_mesh.vert.glsl")),
				std::string(resources::text("shaders/draw_color_mesh.frag.glsl")));
			result->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			result->validate();
			return result;
		}();
		return *program;
	}
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(ColorMesh2D mesh) :
		_mesh(std::move(mesh))
	{
	}
	void DrawColorMeshRenderCommand::execute(RenderContext &context) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &p = _sharedProgram();
		p.activate(context);
		_mesh.layout().activate(context);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
