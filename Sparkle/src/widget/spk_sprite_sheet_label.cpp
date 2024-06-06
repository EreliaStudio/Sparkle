#include "widget/spk_sprite_sheet_label.hpp"

namespace spk
{
	void SpriteSheetLabel::_onGeometryChange()
	{
		_label.setGeometry(anchor(), size());
		_label.setLayer(layer());
	}

	void SpriteSheetLabel::_onRender()
	{
		_label.render();
	}

	SpriteSheetLabel::SpriteSheetLabel(spk::Widget* p_parent) :
		spk::Widget(p_parent)
	{
		_onGeometryChange();
	}

	SpriteSheetLabel::SpriteSheetLabel(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_onGeometryChange();
	}

	WidgetComponent::SpriteSheetRenderer& SpriteSheetLabel::label()
	{
		return _label;
	}

	const WidgetComponent::SpriteSheetRenderer& SpriteSheetLabel::label() const
	{
		return _label;
	}
}