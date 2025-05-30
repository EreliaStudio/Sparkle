#pragma once

#include "widget/spk_container_widget.hpp"
#include "widget/spk_scroll_bar.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class IScrollArea : public spk::Widget
	{
	private:
		bool _isHorizontalScrollBarVisible = true;
		spk::ScrollBar _horizontalScrollBar;
		spk::ScrollBar::Contract _horizontalBarContract;
		bool _isVerticalScrollBarVisible = true;
		spk::ScrollBar _verticalScrollBar;
		spk::ScrollBar::Contract _verticalBarContract;
		spk::SafePointer<spk::Widget> _content;

		uint32_t _scrollBarWidth = 16;

		void _onGeometryChange() override;

		void _onMouseEvent(spk::MouseEvent &p_event) override;

	public:
		IScrollArea(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void setContent(spk::SafePointer<spk::Widget> p_content);
		virtual spk::SafePointer<spk::Widget> content();

		void setScrollBarVisible(spk::ScrollBar::Orientation p_orientation, bool p_state);
		void setScrollBarWidth(const float &p_scrollBarWidth);

		void setContentSize(const spk::Vector2UInt &p_contentSize);
	};

	template <typename TContentType, typename = std::enable_if_t<std::is_base_of<spk::Widget, TContentType>::value>>
	class ScrollArea : public IScrollArea
	{
	private:
		TContentType _contentObject;

		using spk::IScrollArea::setContent;

	public:
		ScrollArea(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			IScrollArea(p_name, p_parent),
			_contentObject(p_name + L"/Container/Content", this)
		{
			setContent(&_contentObject);
			_contentObject.activate();
		}
	};
}