#include "draw_font_render_command.hpp"
#include "program.hpp"
#include "uniform_buffer.hpp"
#include "viewport_uniform_render_command.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
namespace
{
	constexpr auto vertex = R"(#version 460 core
layout(location=0)in vec2 inPosition;layout(location=1)in float inDepth;layout(location=2)in vec2 inUV;layout(std140)uniform ViewportData{mat4 uProjection;};layout(location=0)out vec2 vertexUV;void main(){gl_Position=uProjection*vec4(inPosition,inDepth,1.0);vertexUV=inUV;})";
	constexpr auto fragment = R"(#version 460 core
uniform sampler2D uAtlas;layout(std140)uniform FontRenderData{vec4 uGlyphColor;vec4 uOutlineColor;float uOutlineThickness;};layout(location=0)in vec2 vertexUV;layout(location=0)out vec4 outColor;void main(){const float fillEdge=.5;const float smoothing=.05;float sdf=texture(uAtlas,vertexUV).r;float outlineEdge=fillEdge-uOutlineThickness;float fillAlpha=smoothstep(fillEdge-smoothing,fillEdge+smoothing,sdf);float outlineAlpha=uOutlineThickness>0?smoothstep(outlineEdge-smoothing,outlineEdge+smoothing,sdf):0;vec4 outline=vec4(uOutlineColor.rgb*uOutlineColor.a*outlineAlpha,uOutlineColor.a*outlineAlpha);vec4 fill=vec4(uGlyphColor.rgb*uGlyphColor.a*fillAlpha,uGlyphColor.a*fillAlpha);vec4 p=fill+outline*(1-fill.a);vec3 color=p.a>0?p.rgb/p.a:vec3(0);outColor=vec4(color,p.a);})";
}
namespace spk
{
	Program &DrawFontRenderCommand::_sharedProgram()
	{
		static auto p = []() {
			auto r = std::make_unique<Program>(vertex, fragment);
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
