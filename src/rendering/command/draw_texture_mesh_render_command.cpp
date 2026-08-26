#include "rendering/command/draw_texture_mesh_render_command.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "graphics/opengl/program.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/internal/resource.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawTextureMeshRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_texture_mesh.vert.glsl")),
				std::string(resources::text("shaders/draw_texture_mesh.frag.glsl")));
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
