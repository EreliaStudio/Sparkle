#include "widget/spk_scroll_area.hpp"

namespace spk
{
	void IScrollArea::_onGeometryChange()
	{
		spk::Vector2UInt containerSize = geometry().size;
		spk::Vector2UInt contentSize = _containerWidget.content().upCast<spk::ScrollableWidget>()->requiredSize();
		spk::Vector2Int contentAnchor = 0;
		spk::Vector2UInt delta = 0;

		if (geometry().size.y < contentSize.y)
		{
			containerSize.x -= _scrollBarWidth;
			_verticalScrollBar.activate();
			contentAnchor.y += -_verticalScrollBar.ratio() * static_cast<float>(contentSize.y - containerSize.y);
		}
		else
		{
			_verticalScrollBar.deactivate();
		}

		if (geometry().size.x < contentSize.x)
		{
			containerSize.y -= _scrollBarWidth;
			_horizontalScrollBar.activate();
			contentAnchor.x += -_horizontalScrollBar.ratio() * static_cast<float>(contentSize.x - containerSize.x);
		}
		else
		{
			_horizontalScrollBar.deactivate();
		}

		containerSize = spk::Vector2UInt::min(containerSize, contentSize);

		_containerWidget.setContentAnchor(contentAnchor);
		_containerWidget.setGeometry(0, containerSize);

		_horizontalScrollBar.setGeometry({0, containerSize.y}, {containerSize.x, _scrollBarWidth});
		_horizontalScrollBar.setScale(static_cast<float>(geometry().size.x) / static_cast<float>(contentSize.x));
		
		_verticalScrollBar.setGeometry({containerSize.x, 0}, {_scrollBarWidth, containerSize.y});
		_verticalScrollBar.setScale(static_cast<float>(geometry().size.y) / static_cast<float>(contentSize.y));
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
		_verticalBarContract(_verticalScrollBar.subscribe([&](const float &p_ratio) { requireGeometryUpdate(); })),
		_containerWidget(p_name + L" - Container", this)
	{
		_containerWidget.activate();

		_horizontalScrollBar.setLayer(layer() + 10.0f);
		_verticalScrollBar.setLayer(layer() + 10.0f);
		_containerWidget.setLayer(layer() + 1.0f);

		setScrollBarWidth(16);

		_horizontalScrollBar.setOrientation(ScrollBar::Orientation::Horizontal);
		_verticalScrollBar.setOrientation(ScrollBar::Orientation::Vertical);
	}

	spk::SafePointer<spk::Widget> IScrollArea::container()
	{
		return (&(_containerWidget));
	}

	void IScrollArea::setScrollBarWidth(const float &p_scrollBarWidth)
	{
		_scrollBarWidth = p_scrollBarWidth;
		requireGeometryUpdate();
	}

	void IScrollArea::setContent(spk::SafePointer<ScrollableWidget> p_content)
	{
		_containerWidget.setContent(p_content);
	}

	void IScrollArea::setContentSize(const spk::Vector2UInt &p_contentSize)
	{
		_containerWidget.setContentSize(p_contentSize);
	}
}