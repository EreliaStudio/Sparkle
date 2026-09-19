#pragma once

#include <string>
#include <string_view>

#include "graphics/color.hpp"
#include "graphics/font.hpp"
#include "rendering/command/text_render_command.hpp"
#include "type/alignment.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class TextLabel : public Widget
	{
	private:
		Font *_font = nullptr;
		Font::Text _text;
		Font::Size _textSize{16};
		Color _glyphColor{1.0f, 1.0f, 1.0f, 1.0f};
		Color _outlineColor{0.0f, 0.0f, 0.0f, 1.0f};
		float _depth = 0.0f;
		Alignment _alignment{Alignment::Horizontal::Center, Alignment::Vertical::Center};
		Vector2UInt _padding{0, 0};

		void _updateSizeHint() override;
		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;
		[[nodiscard]] TextRenderCommand::Anchor _textAnchor() const;

	public:
		explicit TextLabel(std::string name, Widget *parent = nullptr);
		TextLabel(std::string name, Font *font, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setFont(Font *font);
		void setText(Font::Text text);
		void setText(std::string_view text);
		void setTextSize(const Font::Size &textSize);
		void setGlyphColor(const Color &color);
		void setOutlineColor(const Color &color);
		void setDepth(float depth);
		void setHorizontalAlignment(Alignment::Horizontal alignment);
		void setVerticalAlignment(Alignment::Vertical alignment);
		void setAlignment(Alignment alignment);
		void setPadding(const Vector2UInt &padding);

		[[nodiscard]] Font *font() noexcept;
		[[nodiscard]] const Font *font() const noexcept;
		[[nodiscard]] const Font::Text &text() const noexcept;
		[[nodiscard]] const Font::Size &textSize() const noexcept;
		[[nodiscard]] const Color &glyphColor() const noexcept;
		[[nodiscard]] const Color &outlineColor() const noexcept;
		[[nodiscard]] float depth() const noexcept;
		[[nodiscard]] Alignment::Horizontal horizontalAlignment() const noexcept;
		[[nodiscard]] Alignment::Vertical verticalAlignment() const noexcept;
		[[nodiscard]] Alignment alignment() const noexcept;
		[[nodiscard]] const Vector2UInt &padding() const noexcept;
	};
}
