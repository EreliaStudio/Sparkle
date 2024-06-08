#include "widget/spk_text_label.hpp"

namespace spk
{
	void TextLabel::_onGeometryChange()
	{
		_box.setGeometry(anchor(), size());
		_box.setLayer(layer());

		_label.setGeometry(anchor() + _box.cornerSize() + _padding, size() - _box.cornerSize() * 2 - _padding * 2);
		_label.setLayer(layer() + 0.01f);
	}

	void TextLabel::_onRender()
	{
		_box.render();
		_label.render();
	}

	TextLabel::TextLabel(Widget* p_parent) :
		TextLabel("Anonymous TextLabel", p_parent)
	{

	}

	TextLabel::TextLabel(const std::string& p_name, Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_padding(0)
	{
		
	}

	spk::Font::Size TextLabel::computeOptimalFontSize(const float& p_ratio)
	{
		if (needGeometryChange() == true)
			applyGeometryChange();
		return (_label.font()->computeOptimalTextSize(_label.text(), p_ratio, _label.size()));
	}

	void TextLabel::setPadding(const spk::Vector2Int& p_padding)
	{
		_padding = p_padding;
		updateGeometry();
	}

	spk::WidgetComponent::FontRenderer& TextLabel::label()
	{
		return (_label);
	}

	spk::WidgetComponent::NineSlicedBox& TextLabel::box()
	{
		return (_box);
	}

	void TextLabel::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
	{
		_box.setSpriteSheet(p_spriteSheet);
	}

	void TextLabel::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_box.setCornerSize(p_cornerSize);
		updateGeometry();
	}	
	
	void TextLabel::setFont(spk::Font* p_font)
	{
		_label.setFont(p_font);
	}

	void TextLabel::setFontSize(const spk::Font::Size& p_fontSize)
	{
		_label.setFontSize(p_fontSize);
	}

	void TextLabel::setText(const std::string& p_text)
	{
		_label.setText(p_text);
	}

	void TextLabel::setTextColor(const spk::Color& p_textColor)
	{
		_label.setTextColor(p_textColor);
	}

	void TextLabel::setTextSize(const size_t& p_textSize)
	{
		_label.setTextSize(p_textSize);
	}

	void TextLabel::setTextEdgeStrenght(const float& p_textEdgeStrenght)
	{
		_label.setTextEdgeStrenght(p_textEdgeStrenght);
	}

	void TextLabel::setOutlineColor(const spk::Color& p_outlineColor)
	{
		_label.setOutlineColor(p_outlineColor);
	}

	void TextLabel::setOutlineSize(const size_t& p_outlineSize)
	{
		_label.setOutlineSize(p_outlineSize);
	}

	void TextLabel::setOutlineEdgeStrenght(const float& p_outlineEdgeStrenght)
	{
		_label.setOutlineEdgeStrenght(p_outlineEdgeStrenght);
	}

	void TextLabel::setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment)
	{
		_label.setVerticalAlignment(p_verticalAlignment);
	}

	void TextLabel::setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment)
	{
		_label.setHorizontalAlignment(p_horizontalAlignment);
	}

}