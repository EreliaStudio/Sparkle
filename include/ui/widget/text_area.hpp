#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "graphics/color.hpp"
#include "graphics/font.hpp"
#include "type/horizontal_alignment.hpp"
#include "type/vertical_alignment.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class TextArea : public Widget
	{
	private:
		Font *_font = nullptr;
		Font::Text _text;
		Font::Size _textSize{16};
		Color _glyphColor{1.0f, 1.0f, 1.0f, 1.0f};
		Color _outlineColor{0.0f, 0.0f, 0.0f, 1.0f};
		float _depth = 0.0f;
		unsigned int _linePadding = 0;
		unsigned int _minimalWidth = 0;
		HorizontalAlignment _horizontalAlignment = HorizontalAlignment::Left;
		VerticalAlignment _verticalAlignment = VerticalAlignment::Top;

		[[nodiscard]] std::vector<Font::Text> _paragraphs() const;
		[[nodiscard]] std::vector<Font::Text> _wrapLines(unsigned int availableWidth) const;
		[[nodiscard]] unsigned int _lineHeight() const;
		[[nodiscard]] unsigned int _naturalWidth() const;
		[[nodiscard]] unsigned int _blockHeight(std::size_t lineCount) const;
		void _updateSizeHint() override;
		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		explicit TextArea(std::string name, Widget *parent = nullptr);
		TextArea(std::string name, Font *font, Widget *parent = nullptr);

		void setFont(Font *font);
		void setText(Font::Text text);
		void setText(std::string_view text);
		void setTextSize(const Font::Size &textSize);
		void setGlyphColor(const Color &color);
		void setOutlineColor(const Color &color);
		void setDepth(float depth);
		void setMinimalWidth(unsigned int width);
		void setLinePadding(unsigned int padding);
		void setHorizontalAlignment(HorizontalAlignment alignment);
		void setVerticalAlignment(VerticalAlignment alignment);
		void setAlignment(HorizontalAlignment horizontal, VerticalAlignment vertical);

		[[nodiscard]] Vector2UInt computePreferredSize(unsigned int availableWidth) const;

		[[nodiscard]] Font *font() noexcept;
		[[nodiscard]] const Font *font() const noexcept;
		[[nodiscard]] const Font::Text &text() const noexcept;
		[[nodiscard]] const Font::Size &textSize() const noexcept;
		[[nodiscard]] const Color &glyphColor() const noexcept;
		[[nodiscard]] const Color &outlineColor() const noexcept;
		[[nodiscard]] float depth() const noexcept;
		[[nodiscard]] unsigned int minimalWidth() const noexcept;
		[[nodiscard]] unsigned int linePadding() const noexcept;
		[[nodiscard]] HorizontalAlignment horizontalAlignment() const noexcept;
		[[nodiscard]] VerticalAlignment verticalAlignment() const noexcept;
	};
}
