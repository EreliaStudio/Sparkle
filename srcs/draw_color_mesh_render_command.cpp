#include "draw_color_mesh_render_command.hpp"
#include "program.hpp"
#include "viewport_uniform_render_command.hpp"
#include <memory>
#include <utility>
namespace
{
	constexpr auto vertex = R"(#version 460 core
layout(location=0)in vec2 inPosition;layout(location=1)in float inDepth;layout(location=2)in vec4 inColor;
layout(std140)uniform ViewportData{mat4 uProjection;};layout(location=0)out vec4 vertexColor;
void main(){gl_Position=uProjection*vec4(inPosition,inDepth,1.0);vertexColor=inColor;})";
	constexpr auto fragment = R"(#version 460 core
layout(location=0)in vec4 vertexColor;layout(location=0)out vec4 outColor;void main(){outColor=vertexColor;})";
}
namespace spk
{
	Program &DrawColorMeshRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(vertex, fragment);
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
