#pragma once

#include "widget/spk_widget.hpp"
#include "widget/spk_scroll_bar.hpp"
#include "widget/spk_container_widget.hpp"

namespace spk
{	
	class ScrollableWidget : public spk::Widget
	{
	private:

	public:
		ScrollableWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent)
		{

		}
		virtual spk::Vector2UInt requiredSize() const = 0;
	};

	class IScrollArea : public spk::Widget
	{
	private:
		spk::ScrollBar _horizontalScrollBar;
		spk::ScrollBar::Contract _horizontalBarContract;
		spk::ScrollBar _verticalScrollBar;
		spk::ScrollBar::Contract _verticalBarContract;
		spk::ContainerWidget _containerWidget;

		float _scrollBarWidth = 16;

		void _onGeometryChange() override;

		void _onMouseEvent(spk::MouseEvent& p_event) override;

	public:
		IScrollArea(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		spk::SafePointer<spk::Widget> container();

		void setScrollBarWidth(const float& p_scrollBarWidth);

		void setContent(spk::SafePointer<ScrollableWidget> p_content);

		void setContentSize(const spk::Vector2UInt& p_contentSize);
	};

	template<typename TContentType,
	         typename = std::enable_if_t<std::is_base_of<ScrollableWidget, TContentType>::value>>
	class ScrollArea : public IScrollArea
	{
	private:
		TContentType _content;

		using spk::IScrollArea::setContent;

	public:
		ScrollArea(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			IScrollArea(p_name, p_parent),
			_content(p_name + L" - Content", container())
		{
			setContent(&_content);
			_content.activate();
		}

		spk::SafePointer<TContentType> content()
		{
			return (&(_content));
		}
	};
}