#include "ui/widget/scroll_area.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace spk
{
	IScrollArea::IScrollArea(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_container(this->name() + ".container", this),
		_horizontalScrollBar(this->name() + ".horizontal-scroll-bar", this),
		_verticalScrollBar(this->name() + ".vertical-scroll-bar", this)
	{
		applyStyle(defaultStyle);
		_verticalScrollBar.setOrientation(Orientation::Vertical);
		_horizontalEditionContract = _horizontalScrollBar.subscribeToEdition([this](float) {
			_updateContentAnchor();
		});
		_verticalEditionContract = _verticalScrollBar.subscribeToEdition([this](float) {
			_updateContentAnchor();
		});
		_updateSizeHint();
		_synchronizeGeometry();
		activate();
	}

	IScrollArea::IScrollArea(std::string name, const SpriteSheet *iconset, Widget *parent) :
		IScrollArea(std::move(name), parent)
	{
		_horizontalScrollBar.setIconset(iconset);
		_verticalScrollBar.setIconset(iconset);
	}

	void IScrollArea::applyStyle(const Style &style)
	{
		_horizontalScrollBar.applyStyle(style);
		_verticalScrollBar.applyStyle(style);
		_updateSizeHint();
	}

	float IScrollArea::_scrollScale(unsigned int viewLength, unsigned int contentLength) const noexcept
	{
		if (contentLength == 0 || contentLength <= viewLength)
		{
			return 1.0f;
		}
		return std::clamp(static_cast<float>(viewLength) / static_cast<float>(contentLength), 0.05f, 1.0f);
	}

	Vector2UInt IScrollArea::_contentMinimalSize() const noexcept
	{
		const Widget *currentContent = _container.content();
		if (currentContent == nullptr)
		{
			return {};
		}

		const auto extent = [](float value) {
			if (!(value > 0.0f))
			{
				return 0u;
			}
			const float maximum = static_cast<float>(std::numeric_limits<unsigned int>::max());
			return static_cast<unsigned int>(std::ceil(std::min(value, maximum)));
		};

		return {extent(currentContent->minimalSize().x), extent(currentContent->minimalSize().y)};
	}

	void IScrollArea::_updateContentAnchor()
	{
		const Vector2UInt size = _container.contentSize();
		const unsigned int horizontalOverflow = size.x > _viewSize.x ? size.x - _viewSize.x : 0;
		const unsigned int verticalOverflow = size.y > _viewSize.y ? size.y - _viewSize.y : 0;
		_container.setContentAnchor({-static_cast<int>(std::lround(static_cast<float>(horizontalOverflow) * _horizontalScrollBar.ratio())), -static_cast<int>(std::lround(static_cast<float>(verticalOverflow) * _verticalScrollBar.ratio()))});
	}

	void IScrollArea::_synchronizeGeometry()
	{
		const Vector2UInt minimumContentSize = _contentMinimalSize();
		bool horizontalVisible = false;
		bool verticalVisible = false;
		for (std::size_t iteration = 0; iteration < 3; ++iteration)
		{
			const unsigned int verticalThickness = verticalVisible ? std::min(_scrollBarWidth, geometry().width) : 0;
			const unsigned int horizontalThickness = horizontalVisible ? std::min(_scrollBarWidth, geometry().height) : 0;
			const Vector2UInt available{
				geometry().width - verticalThickness,
				geometry().height - horizontalThickness};
			const bool nextHorizontalVisible = minimumContentSize.x > available.x;
			const bool nextVerticalVisible = minimumContentSize.y > available.y;
			if (horizontalVisible == nextHorizontalVisible && verticalVisible == nextVerticalVisible)
			{
				break;
			}
			horizontalVisible = nextHorizontalVisible;
			verticalVisible = nextVerticalVisible;
		}

		const bool visibilityChanged =
			_horizontalScrollBarVisible != horizontalVisible ||
			_verticalScrollBarVisible != verticalVisible;
		_horizontalScrollBarVisible = horizontalVisible;
		_verticalScrollBarVisible = verticalVisible;

		const unsigned int verticalThickness = _verticalScrollBarVisible ? std::min(_scrollBarWidth, geometry().width) : 0;
		const unsigned int horizontalThickness = _horizontalScrollBarVisible ? std::min(_scrollBarWidth, geometry().height) : 0;
		_viewSize = {
			geometry().width - verticalThickness,
			geometry().height - horizontalThickness};

		_container.setGeometry({Vector2Int{0, 0}, _viewSize});
		_container.setContentSize({std::max(minimumContentSize.x, _viewSize.x), std::max(minimumContentSize.y, _viewSize.y)});
		if (_horizontalScrollBarVisible)
		{
			_horizontalScrollBar.activate();
			_horizontalScrollBar.setGeometry({Vector2Int{0, static_cast<int>(_viewSize.y)}, Vector2UInt{_viewSize.x, horizontalThickness}});
		}
		else
		{
			_horizontalScrollBar.deactivate();
			_horizontalScrollBar.setGeometry({Vector2Int{0, static_cast<int>(_viewSize.y)}, Vector2UInt{_viewSize.x, 0}});
		}

		if (_verticalScrollBarVisible)
		{
			_verticalScrollBar.activate();
			_verticalScrollBar.setGeometry({Vector2Int{static_cast<int>(_viewSize.x), 0}, Vector2UInt{verticalThickness, _viewSize.y}});
		}
		else
		{
			_verticalScrollBar.deactivate();
			_verticalScrollBar.setGeometry({Vector2Int{static_cast<int>(_viewSize.x), 0}, Vector2UInt{0, _viewSize.y}});
		}

		const Vector2UInt contentSize = _container.contentSize();
		_horizontalScrollBar.setScale(_scrollScale(_viewSize.x, contentSize.x));
		_verticalScrollBar.setScale(_scrollScale(_viewSize.y, contentSize.y));
		_updateContentAnchor();

		if (visibilityChanged)
		{
			_updateSizeHint();
		}
	}

	void IScrollArea::_updateSizeHint()
	{
		SizeHint hint = sizeHint();
		const float verticalThickness = _verticalScrollBarVisible ? static_cast<float>(_scrollBarWidth) : 0.0f;
		const float horizontalThickness = _horizontalScrollBarVisible ? static_cast<float>(_scrollBarWidth) : 0.0f;
		hint.minimal = {verticalThickness, horizontalThickness};
		hint.preferred = hint.minimal;
		if (_horizontalScrollBarVisible)
		{
			hint.minimal.x = std::max(hint.minimal.x, _horizontalScrollBar.minimalSize().x + verticalThickness);
			hint.preferred.x = std::max(hint.preferred.x, _horizontalScrollBar.preferredSize().x + verticalThickness);
		}
		if (_verticalScrollBarVisible)
		{
			hint.minimal.y = std::max(hint.minimal.y, _verticalScrollBar.minimalSize().y + horizontalThickness);
			hint.preferred.y = std::max(hint.preferred.y, _verticalScrollBar.preferredSize().y + horizontalThickness);
		}
		setSizeHint(hint);
	}

	void IScrollArea::_onGeometryChange()
	{
		_synchronizeGeometry();
	}

	void IScrollArea::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event)
	{
		if (!_verticalScrollBarVisible || event.record.value.y == 0.0f || !viewRegion().viewport.contains(event.device.position))
		{
			return;
		}
		_verticalScrollBar.setRatio(_verticalScrollBar.ratio() - event.record.value.y * _verticalScrollBar.step());
		event.consumed = true;
	}

	void IScrollArea::setContent(Widget *content)
	{
		_contentSizeHintContract.resign();
		_container.setContent(content);
		if (content != nullptr)
		{
			_contentSizeHintContract = content->subscribeToSizeHintEdition([this, content](ResizeableTrait *) {
				if (_container.content() == content)
				{
					_synchronizeGeometry();
				}
			});
		}
		_synchronizeGeometry();
	}

	void IScrollArea::setScrollBarWidth(unsigned int width)
	{
		if (_scrollBarWidth == width)
		{
			return;
		}
		_scrollBarWidth = width;
		_updateSizeHint();
		_synchronizeGeometry();
	}

	Widget *IScrollArea::content() noexcept
	{
		return _container.content();
	}

	const Widget *IScrollArea::content() const noexcept
	{
		return _container.content();
	}

	const Vector2UInt &IScrollArea::contentSize() const noexcept
	{
		return _container.contentSize();
	}

	const Vector2UInt &IScrollArea::viewSize() const noexcept
	{
		return _viewSize;
	}

	bool IScrollArea::isScrollBarVisible(Orientation orientation) const noexcept
	{
		return orientation == Orientation::Horizontal ? _horizontalScrollBarVisible : _verticalScrollBarVisible;
	}

	unsigned int IScrollArea::scrollBarWidth() const noexcept
	{
		return _scrollBarWidth;
	}

	ContainerWidget &IScrollArea::container() noexcept
	{
		return _container;
	}

	const ContainerWidget &IScrollArea::container() const noexcept
	{
		return _container;
	}

	ScrollBar &IScrollArea::horizontalScrollBar() noexcept
	{
		return _horizontalScrollBar;
	}

	const ScrollBar &IScrollArea::horizontalScrollBar() const noexcept
	{
		return _horizontalScrollBar;
	}

	ScrollBar &IScrollArea::verticalScrollBar() noexcept
	{
		return _verticalScrollBar;
	}

	const ScrollBar &IScrollArea::verticalScrollBar() const noexcept
	{
		return _verticalScrollBar;
	}
}
