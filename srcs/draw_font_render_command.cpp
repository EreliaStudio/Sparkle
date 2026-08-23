#include "draw_font_render_command.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "program.hpp"
#include "resource.hpp"
#include "uniform_buffer.hpp"
#include "viewport_uniform_render_command.hpp"

namespace spk
{
	Program &DrawFontRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(
				std::string(resources::text("shaders/draw_font.vert.glsl")),
				std::string(resources::text("shaders/draw_font.frag.glsl")));
			r->bindUniformBlock("ViewportData", ViewportUniformRenderCommand::MatrixUBOBindingPoint);
			r->bindUniformBlock("FontRenderData", FontDataUBOBindingPoint);
			r->bindSampler("uAtlas", AtlasSamplerBindingPoint);
			r->validate();
			return r;
		}();
		return *p;
	}
	UniformBuffer &DrawFontRenderCommand::_sharedBuffer()
	{
		static UniformBuffer b(FontDataUBOBindingPoint, sizeof(FontRenderData));
		return b;
	}
	DrawFontRenderCommand::DrawFontRenderCommand(const Font::Atlas *a, TextureMesh2D m, Color g, Color o, float t) :
		_atlas(a),
		_mesh(std::move(m)),
		_data{g, o, t, {}},
		_sampler(AtlasSamplerBindingPoint)
	{
		if (!a)
		{
			throw std::invalid_argument("DrawFontRenderCommand atlas cannot be null");
		}
		_sampler.setTexture(a);
		_sampler.validate();
	}
	void DrawFontRenderCommand::execute(RenderContext &c) const
	{
		if (_mesh.empty())
		{
			return;
		}
		auto &b = _sharedBuffer();
		b.setData(_data);
		b.validate();
		b.activate(c);
		auto &p = _sharedProgram();
		p.activate(c);
		_sampler.activate(c);
		_mesh.layout().activate(c);
		p.render(Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
	}
}
