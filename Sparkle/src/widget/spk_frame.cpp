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

	Frame::Frame(spk::Widget* p_parent = nullptr) :
		spk::Widget(p_parent)
	{

	}

	WidgetComponent::NineSlicedBox& Frame::box()
	{
		return (_box);
	}
}