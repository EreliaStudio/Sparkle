#pragma once

#include "widget/spk_widget.hpp"
#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"

namespace spk
{
	class MultilineTextLabel : public spk::Widget
	{
	private:
		spk::NineSliceRenderer _backgroundRenderer;
		std::vector<spk::FontRenderer> _lineRenderers;
		std::vector<spk::FontRenderer::Contract> _contracts;

		std::wstring _text;
		spk::VerticalAlignment   _verticalAlignment = spk::VerticalAlignment::Centered;
		spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Left;

		spk::Vector2UInt _cornerSize {10, 10};

		void _refreshLineRenderers();

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent &p_event) override;

	public:
		MultilineTextLabel(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		spk::Vector2UInt computeTextSize() const;
		spk::Vector2UInt computeExpectedTextSize(const spk::Font::Size &p_textSize) const;
		spk::Vector2UInt minimalSize() const override;

		const std::wstring &text() const { return _text; }
		const spk::Vector2UInt &cornerSize() const { return _cornerSize; }
		spk::SafePointer<spk::Font> font() const;
		const spk::Font::Size &fontSize() const;

		void setFont(spk::SafePointer<spk::Font> p_font);
		void setText(const std::wstring &p_text);
		void setFontSize(const spk::Font::Size &p_textSize);
		void setTextColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor);
		void setTextAlignment(const spk::HorizontalAlignment &p_hAlign,
						  const spk::VerticalAlignment   &p_vAlign);

		void setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2UInt &p_cornerSize);
	};
}
