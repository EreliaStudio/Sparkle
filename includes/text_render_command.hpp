#pragma once

#include <memory>
#include <string_view>

#include "draw_font_render_command.hpp"
#include "horizontal_alignment.hpp"
#include "vertical_alignment.hpp"

namespace spk
{
	class TextRenderCommand final : public RenderCommand
	{
	private:
		static float _outlineThickness(const Font::Size &);
		static TextureMesh2D _mesh(Font::Atlas &, const Font::Text &, Vector2Int, HorizontalAlignment, VerticalAlignment, float);

		std::unique_ptr<DrawFontRenderCommand> _command;

	public:
		TextRenderCommand(Font *, Font::Size, Font::Text, Vector2Int anchor, Color glyphColor, Color outlineColor = {}, HorizontalAlignment horizontal = HorizontalAlignment::Left, VerticalAlignment vertical = VerticalAlignment::Top, float depth = 0);
		TextRenderCommand(Font *, Font::Size, std::string_view, Vector2Int anchor, Color glyphColor, Color outlineColor = {}, HorizontalAlignment horizontal = HorizontalAlignment::Left, VerticalAlignment vertical = VerticalAlignment::Top, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
