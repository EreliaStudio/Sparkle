#pragma once

#include "widget/spk_widget.hpp"

#include "structure/graphics/renderer/spk_font_renderer.hpp"
#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"

namespace spk
{
	class TextLabel : public spk::Widget
	{
	private:
		spk::NineSliceRenderer _backgroundRenderer;
		spk::FontRenderer _fontRenderer;
		spk::FontRenderer::Contract _onFontResizeContract;

		std::wstring _text = L"";
		spk::VerticalAlignment _verticalAlignment;
		spk::HorizontalAlignment _horizontalAlignment;

		spk::Vector2UInt _cornerSize = {10, 10};

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent &p_event) override;

	public:
		TextLabel(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		spk::Vector2UInt computeTextSize() const;
		spk::Vector2UInt computeExpectedTextSize(const spk::Font::Size &p_textSize) const;
		
		spk::Vector2UInt minimalSize() const override;

		const std::wstring &text() const;
		const spk::Vector2UInt &cornerSize() const;
		spk::SafePointer<spk::Font> font() const;
		const spk::Font::Size &fontSize() const;

		void setFont(spk::SafePointer<spk::Font> p_font);
		void setText(const std::wstring &p_text);
		void setFontSize(const spk::Font::Size &p_textSize);
		void setFontColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor);
		void setTextAlignment(const spk::HorizontalAlignment &p_horizontalAlignment, const spk::VerticalAlignment &p_verticalAlignment);

		void setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2UInt &p_cornerSize);
	};
}