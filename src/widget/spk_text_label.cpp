#include "widget/spk_text_label.hpp"

namespace spk
{
	void TextLabel::_onGeometryChange()
	{
		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(geometry(), layer(), _cornerSize);
		_backgroundRenderer.validate();

		_fontRenderer.clear();
		spk::Vector2Int textAnchor = _fontRenderer.computeTextAnchor(geometry().shrink(_cornerSize), _text, _horizontalAlignment, _verticalAlignment);
		_fontRenderer.prepare(_text, textAnchor, layer() + 0.01f);
		_fontRenderer.validate();
	}

	void TextLabel::_onPaintEvent(spk::PaintEvent& p_event)
	{
		_backgroundRenderer.render();
		_fontRenderer.render();
	}

	TextLabel::TextLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent):
		spk::Widget(p_name, p_parent)
	{

	}

	void TextLabel::setFont(spk::SafePointer<spk::Font> p_font)
	{
		_fontRenderer.setFont(p_font);
		requireGeometryUpdate();
	}

	void TextLabel::setText(const std::wstring& p_text)
	{
		_text = p_text;
		requireGeometryUpdate();
	}

	void TextLabel::setTextSize(const spk::Font::Size& p_textSize)
	{
		_fontRenderer.setFontSize(p_textSize);
		requireGeometryUpdate();
	}

	void TextLabel::setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor)
	{
		_fontRenderer.setGlyphColor(p_glyphColor);
		_fontRenderer.setOutlineColor(p_outlineColor);
	}

	void TextLabel::setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_verticalAlignment = p_verticalAlignment;
		requireGeometryUpdate();
	}

	void TextLabel::setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_backgroundRenderer.setSpriteSheet(p_spriteSheet);
	}

	void TextLabel::setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		requireGeometryUpdate();
	}
}