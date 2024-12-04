#include "widget/spk_frame.hpp"

namespace spk
{
	void Frame::_onGeometryChange()
	{
		_colorBoxComponent.setGeometry(geometry());
		_nineSliceComponent.setGeometry(geometry());
	}

	void Frame::_onPaintEvent(const spk::PaintEvent& p_event)
	{
		if (_nineSliceComponent.texture() == nullptr)
		{
			_colorBoxComponent.render();
		}
		else
		{
			_nineSliceComponent.render();
		}
	}

	Frame::Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_colorBoxComponent.setColors(spk::Color::blue, spk::Color::red);
		_colorBoxComponent.setLayer(0);
		_colorBoxComponent.setCornerSize(spk::Vector2UInt(16, 16));
		
		_nineSliceComponent.setLayer(0);
		_nineSliceComponent.setCornerSize(spk::Vector2UInt(16, 16));
	}

	spk::WidgetComponent::ColorBox& Frame::colorBoxComponent()
	{
		return (_colorBoxComponent);
	}

	const spk::WidgetComponent::ColorBox& Frame::colorBoxComponent() const
	{
		return (_colorBoxComponent);
	}
	
	spk::WidgetComponent::NineSliceRenderer& Frame::nineSliceComponent()
	{
		return (_nineSliceComponent);
	}
	
	const spk::WidgetComponent::NineSliceRenderer& Frame::nineSliceComponent() const
	{
		return (_nineSliceComponent);
	}
}