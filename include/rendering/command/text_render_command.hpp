#pragma once

#include <memory>
#include <string_view>

#include "rendering/command/draw_font_render_command.hpp"
#include "type/alignment.hpp"

namespace spk
{
	class TextRenderCommand final : public RenderCommand
	{
	public:
		struct Anchor
		{
			Vector2Int position;
			Alignment alignment;
		};

	private:
		static float _outlineThickness(const Font::Size &);
		static TextureMesh2D _mesh(Font::Atlas &, const Font::Text &, const Anchor &, float);

		std::unique_ptr<DrawFontRenderCommand> _command;

	public:
		TextRenderCommand(Font *, Font::Size, Font::Text, Anchor, Color glyphColor, Color outlineColor = {}, float depth = 0);
		TextRenderCommand(Font *, Font::Size, std::string_view, Anchor, Color glyphColor, Color outlineColor = {}, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
