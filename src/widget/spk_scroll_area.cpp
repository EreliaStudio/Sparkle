#include "widget/spk_scroll_area.hpp"

namespace spk
{
	void IScrollArea::_onGeometryChange()
    {
		const spk::Vector2UInt minSize   = _content->minimalSize();
		const spk::Vector2UInt viewSize  = geometry().size;

		bool needV = _isVerticalScrollBarVisible   && (minSize.y > viewSize.y);
		bool needH = _isHorizontalScrollBarVisible && (minSize.x > viewSize.x);

		spk::Vector2UInt reserved = viewSize;
		if (needV)
		{
			reserved.x -= std::min(reserved.x, _scrollBarWidth);
		}
		if (needH)
		{
			reserved.y -= std::min(reserved.y, _scrollBarWidth);
		}

		if (!needH && _isHorizontalScrollBarVisible && minSize.x > reserved.x)
		{
			needH = true;
			reserved.y -= std::min(reserved.y, _scrollBarWidth);
		}
		if (!needV && _isVerticalScrollBarVisible && minSize.y > reserved.y)
		{
			needV = true;
			reserved.x -= std::min(reserved.x, _scrollBarWidth);
		}

		const spk::Vector2UInt contentSize = spk::Vector2UInt::max(minSize, reserved);

		needH ? _horizontalScrollBar.activate() : _horizontalScrollBar.deactivate();
		needV ? _verticalScrollBar.activate()   : _verticalScrollBar.deactivate();

		const float hScale = float(reserved.x) / float(contentSize.x);
		const float vScale = float(reserved.y) / float(contentSize.y);

		if (needH)
		{
			_horizontalScrollBar.setGeometry({0, int(reserved.y)}, {reserved.x, _scrollBarWidth});
			_horizontalScrollBar.setScale(hScale);
		}
		if (needV)
		{
			_verticalScrollBar.setGeometry({int(reserved.x), 0}, {_scrollBarWidth, reserved.y});
			_verticalScrollBar.setScale(vScale);
		}

		spk::Vector2Int contentAnchor { 0, 0 };

		if (contentSize.x > reserved.x)
		{
			contentAnchor.x = -int(_horizontalScrollBar.ratio() * (contentSize.x - reserved.x));
		}
		if (contentSize.y > reserved.y)
		{
			contentAnchor.y = -int(_verticalScrollBar.ratio() * (contentSize.y - reserved.y));
		}

		_content->setGeometry({contentAnchor, contentSize});
    }

	void IScrollArea::_onMouseEvent(spk::MouseEvent &p_event)
	{
		if (p_event.type == spk::MouseEvent::Type::Wheel)
		{
			if (p_event.scrollValue < 0 && _verticalScrollBar.ratio() < 1.0f)
			{
				_verticalScrollBar.setRatio(std::min(_verticalScrollBar.ratio() + 0.1f, 1.0f));
			}
			else if (p_event.scrollValue > 0 && _verticalScrollBar.ratio() > 0.0f)
			{
				_verticalScrollBar.setRatio(std::max(_verticalScrollBar.ratio() - 0.1f, 0.0f));
			}
		}
	}

	IScrollArea::IScrollArea(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_horizontalScrollBar(p_name + L"/Horizontal ScrollBar", this),
		_verticalScrollBar(p_name + L"/Vertical ScrollBar", this),
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