#include "widget/spk_frame.hpp"

namespace spk
{
	void Frame::_onGeometryChange()
	{
		_component.setGeometry(geometry());
	}

	void Frame::_onPaintEvent(const spk::PaintEvent& p_event)
	{
		_component.render();
	}

	Frame::Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_component.setColors(spk::Color::blue, spk::Color::red);
		_component.setLayer(0);
		_component.setCornerSize(spk::Vector2UInt(16, 16));
	}
}