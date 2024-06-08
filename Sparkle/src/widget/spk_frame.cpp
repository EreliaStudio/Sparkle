#include "widget/spk_frame.hpp"

namespace spk
{
	void Frame::_onGeometryChange()
	{
		_box.setGeometry(anchor(), size());
		_box.setLayer(layer());
	}

	void Frame::_onRender()
	{
		_box.render();
	}

	Frame::Frame(spk::Widget* p_parent) :
		Frame("Anonymous Frame", p_parent)
	{

	}

	Frame::Frame(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_parent)
	{

	}

	WidgetComponent::NineSlicedBox& Frame::box()
	{
		return (_box);
	}

	const WidgetComponent::NineSlicedBox& Frame::box() const
	{
		return (_box);
	}

    void Frame::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_box.setCornerSize(p_cornerSize);
	}

	void Frame::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
	{
		_box.setSpriteSheet(p_spriteSheet);
	}
}