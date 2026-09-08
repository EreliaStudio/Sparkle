#include "rendering/command/draw_texture_mesh_render_command.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/texture.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawTextureMeshRenderCommand::_sharedProgram()
	{
		static auto program = []() {
			auto result = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_texture_mesh.vert.glsl")),
				std::string(resources::text("shaders/draw_texture_mesh.frag.glsl")));
			result->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			result->bindSampler("uTexture", TextureSamplerBindingPoint);
			result->validate();
			return result;
		}();
		return *program;
	}
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(const Texture *texture, TextureMesh2D mesh) :
		_texture(texture),
		_mesh(std::move(mesh)),
		_sampler(TextureSamplerBindingPoint)
	{
		if (!texture)
		{
			throw std::invalid_argument("DrawTextureMeshRenderCommand texture cannot be null");
		}
		_sampler.setTexture(texture);
		_sampler.validate();
	}
	void DrawTextureMeshRenderCommand::execute(RenderContext &context) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &p = _sharedProgram();
		p.activate(context);
		_sampler.activate(context);
		_mesh.layout().activate(context);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
