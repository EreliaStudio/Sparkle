#include "rendering/command/draw_font_render_command.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "graphics/internal/resource.hpp"
#include "graphics/opengl/mesh_layout.hpp"
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
	DrawFontRenderCommand::DrawFontRenderCommand(Texture::Handle atlas, TextureMesh2D mesh, Color glyphColor, Color outlineColor, float threshold) :
		_mesh(std::move(mesh)),
		_data{glyphColor, outlineColor, threshold, {}},
		_sampler(AtlasSamplerBindingPoint)
	{
		if (!atlas)
		{
			throw std::invalid_argument("DrawFontRenderCommand atlas cannot be null");
		}
		_sampler.setTexture(std::move(atlas));
		_sampler.validate();
	}

	DrawFontRenderCommand::DrawFontRenderCommand(const Font::Atlas *atlas, TextureMesh2D mesh, Color glyphColor, Color outlineColor, float threshold) :
		DrawFontRenderCommand(atlas ? atlas->handle() : Texture::Handle{}, std::move(mesh), glyphColor, outlineColor, threshold)
	{
	}
	void DrawFontRenderCommand::execute(RenderContext &context) const
	{
		if (_mesh.empty())
		{
			return;
		}
		static thread_local auto cpu = [] {
			Buffer::Builder builder;
			builder.resize<FontRenderData>(1);
			return std::move(builder).buildMutable();
		}();
		static thread_local UniformBuffer b(FontDataUBOBindingPoint, sizeof(FontRenderData));
		if (b.buffer() == nullptr)
		{
			b.bind(&cpu);
		}
		cpu.cast<FontRenderData>()[0] = _data;
		cpu.validate();
		b.activate(context);
		auto &p = _sharedProgram();
		p.activate(context);
		_sampler.activate(context);
		MeshLayout::obtain(_mesh).activate(context);
		p.render(Program::Primitive::Triangles, IndexBuffer::Type::UnsignedInt, 0, _mesh.indexCount());
	}
}
