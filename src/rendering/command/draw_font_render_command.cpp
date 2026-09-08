#include "rendering/command/draw_font_render_command.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/uniform_buffer.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawFontRenderCommand::_sharedProgram()
	{
		static auto program = []() {
			auto result = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_font.vert.glsl")),
				std::string(resources::text("shaders/draw_font.frag.glsl")));
			result->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			result->bindUniformBlock("FontRenderData", FontDataUBOBindingPoint);
			result->bindSampler("uAtlas", AtlasSamplerBindingPoint);
			result->validate();
			return result;
		}();
		return *program;
	}
	UniformBuffer &DrawFontRenderCommand::_sharedBuffer()
	{
		static UniformBuffer buffer(FontDataUBOBindingPoint, sizeof(FontRenderData));
		return buffer;
	}
	DrawFontRenderCommand::DrawFontRenderCommand(const Font::Atlas *atlas, TextureMesh2D mesh, Color glyphColor, Color outlineColor, float threshold) :
		_atlas(atlas),
		_mesh(std::move(mesh)),
		_data{glyphColor, outlineColor, threshold, {}},
		_sampler(AtlasSamplerBindingPoint)
	{
		if (!atlas)
		{
			throw std::invalid_argument("DrawFontRenderCommand atlas cannot be null");
		}
		_sampler.setTexture(atlas);
		_sampler.validate();
	}
	void DrawFontRenderCommand::execute(RenderContext &context) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &b = _sharedBuffer();
		b.setData(_data);
		b.validate();
		b.activate(context);
		auto &p = _sharedProgram();
		p.activate(context);
		_sampler.activate(context);
		_mesh.layout().activate(context);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
