#include "draw_texture_mesh_render_command.hpp"
#include "program.hpp"
#include "texture.hpp"
#include "viewport_uniform_render_command.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
namespace
{
	constexpr auto vertex = R"(#version 460 core
layout(location=0)in vec2 inPosition;layout(location=1)in float inDepth;layout(location=2)in vec2 inUV;
layout(std140)uniform ViewportData{mat4 uProjection;};layout(location=0)out vec2 vertexUV;
void main(){gl_Position=uProjection*vec4(inPosition,inDepth,1.0);vertexUV=inUV;})";
	constexpr auto fragment = R"(#version 460 core
uniform sampler2D uTexture;layout(location=0)in vec2 vertexUV;layout(location=0)out vec4 outColor;void main(){outColor=texture(uTexture,vertexUV);})";
}
namespace spk
{
	Program &DrawTextureMeshRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(vertex, fragment);
			r->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			r->bindSampler("uTexture", TextureSamplerBindingPoint);
			r->validate();
			return r;
		}();
		return *p;
	}
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(const Texture *t, TextureMesh2D m) :
		_texture(t),
		_mesh(std::move(m)),
		_sampler(TextureSamplerBindingPoint)
	{
		if (!t)
		{
			throw std::invalid_argument("DrawTextureMeshRenderCommand texture cannot be null");
		}
		_sampler.setTexture(t);
		_sampler.validate();
	}
	void DrawTextureMeshRenderCommand::execute(RenderContext &c) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &p = _sharedProgram();
		p.activate(c);
		_sampler.activate(c);
		_mesh.layout().activate(c);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
