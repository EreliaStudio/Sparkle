#include "text_render_command.hpp"
#include <stdexcept>
namespace spk
{
	float TextRenderCommand::_outlineThickness(const Font::Size &s)
	{
		if (!s.outline)
		{
			return 0;
		}
		const float padding = float(s.outline + 2);
		return float(s.outline) * 128 / (padding * 255);
	}
	TextureMesh2D TextRenderCommand::_mesh(Font::Atlas &a, const Font::Text &text, const Anchor &anchor, float depth)
	{
		a.loadGlyphs(text);
		const auto size = a.computeStringSize(text);
		auto baseline = a.computeStringBaselineOffset(text);
		int left = anchor.position.x, top = anchor.position.y;
		if (anchor.horizontalAlignment == HorizontalAlignment::Center)
		{
			left -= int(size.x) / 2;
		}
		else if (anchor.horizontalAlignment == HorizontalAlignment::Right)
		{
			left -= int(size.x);
		}
		if (anchor.verticalAlignment == VerticalAlignment::Center)
		{
			top -= int(size.y) / 2;
		}
		else if (anchor.verticalAlignment == VerticalAlignment::Bottom)
		{
			top -= int(size.y);
		}
		int cursor = left + baseline.x, base = top + baseline.y;
		TextureMesh2D::Builder b;
		for (auto cp : text)
		{
			const auto &g = a.glyph(cp);
			if (g.size.x && g.size.y)
			{
				auto vertex = [&](int i) {
					return TextureMesh2D::Vertex{{float(cursor + g.positions[i].x), float(base + g.positions[i].y)}, depth, g.uvs[i]};
				};
				b.addShape(vertex(0), vertex(1), vertex(3), vertex(2));
			}
			cursor += g.step.x;
		}
		return std::move(b).build();
	}
	TextRenderCommand::TextRenderCommand(Font *f, Font::Size s, Font::Text text, Anchor anchor, Color glyph, Color outline, float depth)
	{
		if (!f)
		{
			throw std::invalid_argument("TextRenderCommand font cannot be null");
		}
		auto &a = f->atlas(s);
		_command = std::make_unique<DrawFontRenderCommand>(&a, _mesh(a, text, anchor, depth), glyph, outline, _outlineThickness(s));
	}
	TextRenderCommand::TextRenderCommand(Font *f, Font::Size s, std::string_view text, Anchor anchor, Color glyph, Color outline, float depth) :
		TextRenderCommand(f, s, Font::textFromUTF8(text), anchor, glyph, outline, depth)
	{
	}
	void TextRenderCommand::execute(RenderContext &c) const
	{
		_command->execute(c);
	}
}
