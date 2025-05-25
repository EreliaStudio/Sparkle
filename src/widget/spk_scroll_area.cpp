#include "widget/spk_scroll_area.hpp"

namespace spk
{
	void IScrollArea::_onGeometryChange()
    {
        if (_content == nullptr)
		{
            return;
		}

		_content->setGeometry({0, 0}, geometry().size);

		_horizontalScrollBar.deactivate();
		_verticalScrollBar.deactivate();
    }

	void IScrollArea::_onMouseEvent(spk::MouseEvent &p_event)
	{
		if (p_event.type == spk::MouseEvent::Type::Wheel)
		{
			if (p_event.scrollValue < 0)
			{
				_verticalScrollBar.setRatio(_verticalScrollBar.ratio() + 0.1f);
			}
			else if (p_event.scrollValue > 0)
			{
				_verticalScrollBar.setRatio(_verticalScrollBar.ratio() - 0.1f);
			}
		}
	}

	IScrollArea::IScrollArea(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_horizontalScrollBar(p_name + L" - Horizontal ScrollBar", this),
		_verticalScrollBar(p_name + L" - Vertical ScrollBar", this),
		_horizontalBarContract(_horizontalScrollBar.subscribe([&](const float &p_ratio) { requireGeometryUpdate(); })),
		_verticalBarContract(_verticalScrollBar.subscribe([&](const float &p_ratio) { requireGeometryUpdate(); }))
	{
		_horizontalScrollBar.setLayer(layer() + 10.0f);
		_verticalScrollBar.setLayer(layer() + 10.0f);

		setScrollBarWidth(16);

		_horizontalScrollBar.setOrientation(ScrollBar::Orientation::Horizontal);
		_verticalScrollBar.setOrientation(ScrollBar::Orientation::Vertical);
	}

	spk::SafePointer<spk::Widget> IScrollArea::content()
	{
		return (_content);
	}

	void IScrollArea::setScrollBarVisible(spk::ScrollBar::Orientation p_orientation, bool p_state)
	{
		switch (p_orientation)
		{
			case spk::ScrollBar::Orientation::Horizontal:
			{
				_isHorizontalScrollBarVisible = p_state;
				break;
			}
			case spk::ScrollBar::Orientation::Vertical:
			{
				_isVerticalScrollBarVisible = p_state;
				break;
			}
		}

		requireGeometryUpdate();
	}

	void IScrollArea::setScrollBarWidth(const float &p_scrollBarWidth)
	{
		_scrollBarWidth = p_scrollBarWidth;
		requireGeometryUpdate();
	}

	void IScrollArea::setContent(spk::SafePointer<spk::Widget> p_content)
	{
		_content = p_content;
	}
}