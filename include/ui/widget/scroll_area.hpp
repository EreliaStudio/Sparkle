#pragma once

#include <concepts>
#include <string>
#include <utility>

#include "ui/widget/container_widget.hpp"
#include "ui/widget/scroll_bar.hpp"

namespace spk
{
	class IScrollArea : public Widget
	{
	private:
		ContainerWidget _container;
		ScrollBar _horizontalScrollBar;
		ScrollBar _verticalScrollBar;
		ScrollBar::EditionContract _horizontalEditionContract;
		ScrollBar::EditionContract _verticalEditionContract;
		Vector2UInt _viewSize{};
		unsigned int _scrollBarWidth = 16;
		bool _horizontalScrollBarVisible = true;
		bool _verticalScrollBarVisible = true;

		[[nodiscard]] float _scrollScale(unsigned int viewLength, unsigned int contentLength) const noexcept;
		void _updateContentAnchor();
		void _synchronizeGeometry();
		void _updateSizeHint() override;
		void _onGeometryChange() override;
		void _onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event) override;

	public:
		explicit IScrollArea(std::string name, Widget *parent = nullptr);
		IScrollArea(std::string name, const SpriteSheet *iconset, Widget *parent = nullptr);

		void setContent(Widget *content);
		void setContentSize(const Vector2UInt &size);
		void setScrollBarVisible(Orientation orientation, bool visible);
		void setScrollBarWidth(unsigned int width);

		[[nodiscard]] Widget *content() noexcept;
		[[nodiscard]] const Widget *content() const noexcept;
		[[nodiscard]] const Vector2UInt &contentSize() const noexcept;
		[[nodiscard]] const Vector2UInt &viewSize() const noexcept;
		[[nodiscard]] bool isScrollBarVisible(Orientation orientation) const noexcept;
		[[nodiscard]] unsigned int scrollBarWidth() const noexcept;

		[[nodiscard]] ContainerWidget &container() noexcept;
		[[nodiscard]] const ContainerWidget &container() const noexcept;
		[[nodiscard]] ScrollBar &horizontalScrollBar() noexcept;
		[[nodiscard]] const ScrollBar &horizontalScrollBar() const noexcept;
		[[nodiscard]] ScrollBar &verticalScrollBar() noexcept;
		[[nodiscard]] const ScrollBar &verticalScrollBar() const noexcept;
	};

	template <typename TContent>
	concept ScrollAreaContent = std::derived_from<TContent, Widget> && std::constructible_from<TContent, std::string, Widget *>;

	template <ScrollAreaContent TContent>
	class ScrollArea : public IScrollArea
	{
	private:
		TContent _contentObject;

	public:
		explicit ScrollArea(std::string name, Widget *parent = nullptr) :
			IScrollArea(std::move(name), parent),
			_contentObject(this->name() + ".content", &container())
		{
			setContent(&_contentObject);
		}

		ScrollArea(std::string name, const SpriteSheet *iconset, Widget *parent = nullptr) :
			IScrollArea(std::move(name), iconset, parent),
			_contentObject(this->name() + ".content", &container())
		{
			setContent(&_contentObject);
		}

		[[nodiscard]] TContent &contentObject() noexcept
		{
			return _contentObject;
		}

		[[nodiscard]] const TContent &contentObject() const noexcept
		{
			return _contentObject;
		}
	};
}
