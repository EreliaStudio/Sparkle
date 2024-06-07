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
		return (_label.font()->computeOptimalTextSize(_label.text(), p_ratio, _label.size()));
	}

	void TextLabel::setPadding(const spk::Vector2Int& p_padding)
	{
		_padding = p_padding;
		setGeometry(anchor(), size());
	}

	spk::WidgetComponent::FontRenderer& TextLabel::label()
	{
		return (_label);
	}

	spk::WidgetComponent::NineSlicedBox& TextLabel::box()
	{
		return (_box);
	}
}