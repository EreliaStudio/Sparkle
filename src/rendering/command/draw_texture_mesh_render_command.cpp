#include "rendering/command/draw_texture_mesh_render_command.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/state.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/uniform.hpp"
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
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(Texture::Handle texture, TextureMesh2D mesh, bool presentSRGB) :
		_mesh(std::move(mesh)),
		_sampler(TextureSamplerBindingPoint),
		_presentSRGB(presentSRGB)
	{
		if (!texture)
		{
			throw std::invalid_argument("DrawTextureMeshRenderCommand texture cannot be null");
		}
		_sampler.setTexture(std::move(texture));
		_sampler.validate();
	}

	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(const Texture *texture, TextureMesh2D mesh, bool presentSRGB) :
		DrawTextureMeshRenderCommand(texture ? texture->handle() : Texture::Handle{}, std::move(mesh), presentSRGB)
	{
	}
	void DrawTextureMeshRenderCommand::execute(RenderContext &context) const
	{
		if (_mesh.empty())
		{
			return;
		}
		const bool srgbTarget = _presentSRGB && OpenGL::State::ColorOutput::destinationIsSRGB(context);
		std::optional<OpenGL::State::ColorOutput::SRGBScope> encoding;
		if (_presentSRGB)
		{
			encoding.emplace(context, srgbTarget);
		}
		auto &p = _sharedProgram();
		p.activate(context);
		Uniform<bool>("uEncodeSRGB", _presentSRGB && !srgbTarget).activate(context);
		_sampler.activate(context);
		_mesh.layout().activate(context);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
