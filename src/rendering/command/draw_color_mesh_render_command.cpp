#include "rendering/command/draw_color_mesh_render_command.hpp"

#include <memory>
#include <utility>

#include "graphics/opengl/program.hpp"
#include "graphics/internal/resource.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawColorMeshRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_color_mesh.vert.glsl")),
				std::string(resources::text("shaders/draw_color_mesh.frag.glsl")));
			r->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			r->validate();
			return r;
		}();
		return *p;
	}
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(ColorMesh2D m) :
		_mesh(std::move(m))
	{
	}
	void DrawColorMeshRenderCommand::execute(RenderContext &c) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &p = _sharedProgram();
		p.activate(c);
		_mesh.layout().activate(c);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
