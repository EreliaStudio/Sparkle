#include "ui/widget/interface_window.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace
{
	unsigned int dimension(float value) noexcept
	{
		return value <= 0.0f ? 0 : static_cast<unsigned int>(std::ceil(value));
	}

	unsigned int reduced(unsigned int value, unsigned int amount) noexcept
	{
		return value > amount ? value - amount : 0;
	}
}

namespace spk
{
	IInterfaceWindow::MenuBar::MenuBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_title(this->name() + ".title", this),
		_minimizeButton(this->name() + ".minimize", this),
		_maximizeButton(this->name() + ".maximize", this),
		_closeButton(this->name() + ".close", this)
	{
		applyStyle(defaultStyle);
		_closeButton.setIconSpriteID(0);
		_maximizeButton.setIconSpriteID(1);
		_minimizeButton.setIconSpriteID(3);
		_layoutReady = true;
		_updateSizeHint();
		activate();
	}

	void IInterfaceWindow::MenuBar::applyStyle(const Style &style)
	{
		_title.applyStyle(style);
		_minimizeButton.applyStyle(style);
		_maximizeButton.applyStyle(style);
		_closeButton.applyStyle(style);
		_title.setTextSize(style.interfaceWindowMenuTitleTextSize);
		_title.setGlyphColor(style.interfaceWindowMenuTitleGlyphColor);
		_title.setOutlineColor(style.interfaceWindowMenuTitleOutlineColor);
		_title.setAlignment(style.interfaceWindowMenuTitleAlignment);
		for (IconButton *button : {&_minimizeButton, &_maximizeButton, &_closeButton})
		{
			button->setIconSize(style.interfaceWindowMenuButtonIconSize);
			button->setIconPadding(style.interfaceWindowMenuButtonIconPadding);
			button->releasedBackground().setCornerSize(style.interfaceWindowMenuButtonCornerSize);
			button->pressedBackground().setCornerSize(style.interfaceWindowMenuButtonCornerSize);
		}
		_updateSizeHint();
		_onGeometryChange();
	}

	IconButton &IInterfaceWindow::MenuBar::_button(Button button)
	{
		switch (button)
		{
		case Button::Minimize:
			return _minimizeButton;
		case Button::Maximize:
			return _maximizeButton;
		case Button::Close:
			return _closeButton;
		}
		throw std::logic_error("Unknown interface-window menu button");
	}

	const IconButton &IInterfaceWindow::MenuBar::_button(Button button) const
	{
		return const_cast<MenuBar *>(this)->_button(button);
	}

	void IInterfaceWindow::MenuBar::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}
		Vector2 minimum = _title.minimalSize();
		minimum.x += static_cast<float>(2 * _margin);
		minimum.y += static_cast<float>(2 * _margin);
		for (const IconButton *button : {&_minimizeButton, &_maximizeButton, &_closeButton})
		{
			if (!button->isActive())
			{
				continue;
			}
			const float side = std::max(button->minimalSize().x, button->minimalSize().y);
			minimum.x += side;
			minimum.y = std::max(minimum.y, side + static_cast<float>(2 * _margin));
		}
		SizeHint hint = sizeHint();
		hint.minimal = minimum;
		hint.preferred = minimum;
		setSizeHint(hint);
		_onGeometryChange();
	}

	void IInterfaceWindow::MenuBar::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}
		const unsigned int innerHeight = reduced(geometry().height, 2 * _margin);
		int cursor = static_cast<int>(reduced(geometry().width, _margin));
		for (IconButton *button : {&_closeButton, &_maximizeButton, &_minimizeButton})
		{
			if (!button->isActive())
			{
				continue;
			}
			const unsigned int minimumSide = dimension(std::max(button->minimalSize().x, button->minimalSize().y));
			const unsigned int side = std::max(innerHeight, minimumSide);
			cursor -= static_cast<int>(side);
			const int y = static_cast<int>(_margin + (innerHeight > side ? (innerHeight - side) / 2 : 0));
			button->setGeometry({Vector2Int{cursor, y}, Vector2UInt{side, side}});
		}
		const unsigned int titleWidth = cursor > static_cast<int>(_margin) ? static_cast<unsigned int>(cursor - static_cast<int>(_margin)) : 0;
		_title.setGeometry({Vector2Int{static_cast<int>(_margin), static_cast<int>(_margin)}, Vector2UInt{titleWidth, innerHeight}});
	}

	void IInterfaceWindow::MenuBar::setTitle(std::string_view title)
	{
		_title.setText(title);
	}

	void IInterfaceWindow::MenuBar::setIconset(const SpriteSheet *iconset)
	{
		_minimizeButton.setIconset(iconset);
		_maximizeButton.setIconset(iconset);
		_closeButton.setIconset(iconset);
	}

	void IInterfaceWindow::MenuBar::setButtonEnabled(Button button, bool enabled)
	{
		IconButton &target = _button(button);
		enabled ? target.activate() : target.deactivate();
		_updateSizeHint();
	}

	void IInterfaceWindow::MenuBar::setMargin(unsigned int margin)
	{
		if (_margin == margin)
		{
			return;
		}
		_margin = margin;
		_updateSizeHint();
	}

	bool IInterfaceWindow::MenuBar::isButtonEnabled(Button button) const
	{
		return _button(button).isActive();
	}
	unsigned int IInterfaceWindow::MenuBar::margin() const noexcept
	{
		return _margin;
	}
	TextLabel &IInterfaceWindow::MenuBar::titleLabel() noexcept
	{
		return _title;
	}
	const TextLabel &IInterfaceWindow::MenuBar::titleLabel() const noexcept
	{
		return _title;
	}
	IconButton &IInterfaceWindow::MenuBar::minimizeButton() noexcept
	{
		return _minimizeButton;
	}
	const IconButton &IInterfaceWindow::MenuBar::minimizeButton() const noexcept
	{
		return _minimizeButton;
	}
	IconButton &IInterfaceWindow::MenuBar::maximizeButton() noexcept
	{
		return _maximizeButton;
	}
	const IconButton &IInterfaceWindow::MenuBar::maximizeButton() const noexcept
	{
		return _maximizeButton;
	}
	IconButton &IInterfaceWindow::MenuBar::closeButton() noexcept
	{
		return _closeButton;
	}
	const IconButton &IInterfaceWindow::MenuBar::closeButton() const noexcept
	{
		return _closeButton;
	}

	IInterfaceWindow::IInterfaceWindow(std::string name, Widget *parent) :
		ScalableWidget(std::move(name), parent),
		_normalBackground(this->name() + ".normal-background", this),
		_minimizedBackground(this->name() + ".minimized-background", this),
		_menuBar(this->name() + ".menu", this)
	{
		applyStyle(defaultStyle);
		_normalBackground.setZOrder(0.0f);
		_minimizedBackground.setZOrder(0.0f);
		_menuBar.setZOrder(1.0f);
		_minimizedBackground.deactivate();
		_minimizeContract = _menuBar.minimizeButton().subscribeToClick([this]() {
			minimize();
		});
		_maximizeContract = _menuBar.maximizeButton().subscribeToClick([this]() {
			maximize();
		});
		_layoutReady = true;
		_updateSizeHint();
		_onGeometryChange();
		activate();
	}

	void IInterfaceWindow::applyStyle(const Style &style)
	{
		if (style.darkNineSlice != nullptr)
		{
			_normalBackground.setSpriteSheet(style.darkNineSlice.get());
		}
		if (style.darkerNineSlice != nullptr)
		{
			_minimizedBackground.setSpriteSheet(style.darkerNineSlice.get());
		}
		_normalBackground.setCornerSize(style.interfaceWindowBackgroundCornerSize);
		_minimizedBackground.setCornerSize(style.interfaceWindowBackgroundCornerSize);
		_menuBar.applyStyle(style);
	}

	IInterfaceWindow::Padding IInterfaceWindow::_effectivePadding() const noexcept
	{
		if (_contentPadding.has_value())
		{
			return *_contentPadding;
		}
		const Vector2Int corner = _normalBackground.cornerSize();
		return {
			.left = static_cast<unsigned int>(std::max(0, corner.x + 2)),
			.right = static_cast<unsigned int>(std::max(0, corner.x + 2)),
			.top = 0,
			.bottom = static_cast<unsigned int>(std::max(0, corner.y + 2))};
	}

	unsigned int IInterfaceWindow::_effectiveMenuHeight() const noexcept
	{
		return std::max(_menuHeight, dimension(_menuBar.minimalSize().y));
	}

	Vector2UInt IInterfaceWindow::_availableContentSize() const noexcept
	{
		const Padding padding = _effectivePadding();
		return {
			reduced(geometry().width, padding.left + padding.right),
			reduced(geometry().height, _effectiveMenuHeight() + padding.top + padding.bottom)};
	}

	Rect2D IInterfaceWindow::_maximizedGeometry() const
	{
		if (!hasParent())
		{
			return {};
		}
		const ViewRegion &parentRegion = parent()->viewRegion();
		return {
			parentRegion.scissor.anchor - parentRegion.viewport.anchor,
			parentRegion.scissor.size};
	}

	void IInterfaceWindow::_applyMinimizedState()
	{
		if (_minimized)
		{
			_normalBackground.deactivate();
			_minimizedBackground.activate();
		}
		else
		{
			_minimizedBackground.deactivate();
			_normalBackground.activate();
		}
	}

	void IInterfaceWindow::_fitMaximizedGeometry()
	{
		if (!_maximized)
		{
			return;
		}
		if (!hasParent())
		{
			_synchronizeMaximizedState();
			return;
		}
		const Rect2D expectedGeometry = _maximizedGeometry();
		if (geometry() != expectedGeometry)
		{
			_setGeometryWithoutConstraints(expectedGeometry);
		}
	}

	void IInterfaceWindow::_synchronizeMaximizedState()
	{
		if (!_maximized)
		{
			return;
		}
		const Rect2D expectedGeometry = _maximizedGeometry();
		if (!hasParent() || geometry() != expectedGeometry)
		{
			_maximized = false;
			_menuBar.maximizeButton().setIconSpriteID(_maximizeSpriteID);
		}
	}

	void IInterfaceWindow::_restore()
	{
		if (!_maximized)
		{
			return;
		}
		_maximized = false;
		_menuBar.maximizeButton().setIconSpriteID(_maximizeSpriteID);
		setGeometry(_restoredGeometry);
	}

	void IInterfaceWindow::_beginMove(EventBase &event, Mouse::Button button, const Vector2Int &position)
	{
		if (button != Mouse::Button::Left || _moving || _maximized || !_menuBar.titleLabel().viewRegion().viewport.contains(position))
		{
			return;
		}
		_moving = true;
		_moveOffset = position - viewRegion().viewport.anchor;
		event.takeFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
		// TODO: Request the Hand cursor once Sparkle exposes cursor-shape requests.
	}

	void IInterfaceWindow::_endMove(EventBase &event)
	{
		if (!_moving)
		{
			return;
		}
		_moving = false;
		event.releaseFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
	}

	void IInterfaceWindow::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}
		const Padding padding = _effectivePadding();
		Vector2 contentMinimum = _minimumContentSize;
		if (_content != nullptr)
		{
			contentMinimum.x = std::max(contentMinimum.x, _content->minimalSize().x);
			contentMinimum.y = std::max(contentMinimum.y, _content->minimalSize().y);
		}
		SizeHint hint = sizeHint();
		hint.minimal = {
			std::max(_menuBar.minimalSize().x, contentMinimum.x + static_cast<float>(padding.left + padding.right)),
			contentMinimum.y + static_cast<float>(_effectiveMenuHeight() + padding.top + padding.bottom)};
		setSizeHint(hint);
		_onGeometryChange();
	}

	void IInterfaceWindow::_updateState(UpdateContext &)
	{
		_fitMaximizedGeometry();
	}

	void IInterfaceWindow::_onGeometryChange()
	{
		_synchronizeMaximizedState();
		if (!_maximized)
		{
			ScalableWidget::_onGeometryChange();
		}
		if (!_layoutReady)
		{
			return;
		}
		const unsigned int menuHeight = _effectiveMenuHeight();
		const Rect2D fill{Vector2Int{0, 0}, geometry().size};
		_normalBackground.setGeometry(fill);
		_minimizedBackground.setGeometry({Vector2Int{0, 0}, Vector2UInt{geometry().width, std::min(menuHeight, geometry().height)}});
		_menuBar.setGeometry({Vector2Int{0, 0}, Vector2UInt{geometry().width, std::min(menuHeight, geometry().height)}});
		const Padding padding = _effectivePadding();
		const Vector2UInt contentSize = _availableContentSize();
		if (_content != nullptr)
		{
			_content->setGeometry({Vector2Int{static_cast<int>(padding.left), static_cast<int>(menuHeight + padding.top)}, contentSize});
		}
		if (_lastContentSize != contentSize)
		{
			_lastContentSize = contentSize;
			_resizeProvider.trigger(contentSize);
		}
	}

	void IInterfaceWindow::_onWindowResizedEvent(WindowResizedEvent &)
	{
		_fitMaximizedGeometry();
	}

	void IInterfaceWindow::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		ScalableWidget::_onWindowFocusLostEvent(event);
		_endMove(event);
	}

	void IInterfaceWindow::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		if (!_maximized)
		{
			ScalableWidget::_onMouseMovedEvent(event);
		}
		if (event.consumed || !_moving)
		{
			return;
		}
		const Vector2Int parentAnchor = hasParent() ? parent()->viewRegion().viewport.anchor : Vector2Int{};
		setGeometry({event.device.position - _moveOffset - parentAnchor, geometry().size});
		event.consumed = true;
	}

	void IInterfaceWindow::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (!_maximized)
		{
			ScalableWidget::_onMouseButtonPressedEvent(event);
		}
		if (event.consumed)
		{
			return;
		}
		_beginMove(event, event.record.button, event.device.position);
		if (!event.consumed && event.record.button == Mouse::Button::Left && viewRegion().viewport.contains(event.device.position))
		{
			event.consumed = true;
		}
	}

	void IInterfaceWindow::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (!_maximized)
		{
			ScalableWidget::_onMouseButtonReleasedEvent(event);
		}
		if (!event.consumed && event.record.button == Mouse::Button::Left)
		{
			_endMove(event);
		}
	}

	void IInterfaceWindow::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event)
	{
		if (!_maximized)
		{
			ScalableWidget::_onMouseButtonDoubleClickedEvent(event);
		}
		if (!event.consumed)
		{
			_beginMove(event, event.record.button, event.device.position);
		}
		if (!event.consumed && event.record.button == Mouse::Button::Left && viewRegion().viewport.contains(event.device.position))
		{
			event.consumed = true;
		}
	}

	void IInterfaceWindow::setContent(Widget *content)
	{
		if (content != nullptr && content->parent() != &_normalBackground)
		{
			throw std::invalid_argument("Interface-window content must be a child of its normal background");
		}
		_contentParentEditionContract.resign();
		_contentSizeHintEditionContract.resign();
		_content = content;
		if (_content != nullptr)
		{
			Widget *observed = _content;
			_contentParentEditionContract = _content->subscribeToParentEdition([this, observed](const Widget *parent) {
				if (_content == observed && parent != &_normalBackground)
				{
					_content = nullptr;
					_contentSizeHintEditionContract.resign();
					_updateSizeHint();
					_onGeometryChange();
				}
			});
			_contentSizeHintEditionContract = _content->subscribeToSizeHintEdition([this](ResizeableTrait *) {
				_updateSizeHint();
			});
		}
		_updateSizeHint();
		_onGeometryChange();
	}

	void IInterfaceWindow::setTitle(std::string_view title)
	{
		_menuBar.setTitle(title);
	}
	void IInterfaceWindow::setMenuHeight(unsigned int height)
	{
		_menuHeight = height;
		_updateSizeHint();
		_onGeometryChange();
	}
	void IInterfaceWindow::setContentPadding(const Padding &padding)
	{
		_contentPadding = padding;
		_updateSizeHint();
		_onGeometryChange();
	}
	void IInterfaceWindow::resetContentPadding()
	{
		_contentPadding.reset();
		_updateSizeHint();
		_onGeometryChange();
	}
	void IInterfaceWindow::setMinimumContentSize(const Vector2 &size)
	{
		_minimumContentSize = {std::max(0.0f, size.x), std::max(0.0f, size.y)};
		_updateSizeHint();
	}
	void IInterfaceWindow::setMenuButtonEnabled(MenuBar::Button button, bool enabled)
	{
		_menuBar.setButtonEnabled(button, enabled);
		_updateSizeHint();
		_onGeometryChange();
	}

	void IInterfaceWindow::setMaximizeSpriteIDs(std::size_t maximizeSpriteID, std::size_t restoreSpriteID)
	{
		_maximizeSpriteID = maximizeSpriteID;
		_restoreSpriteID = restoreSpriteID;
		_menuBar.maximizeButton().setIconSpriteID(_maximized ? _restoreSpriteID : _maximizeSpriteID);
	}

	void IInterfaceWindow::minimize()
	{
		_restore();
		_minimized = !_minimized;
		_applyMinimizedState();
	}

	void IInterfaceWindow::maximize()
	{
		if (_maximized)
		{
			_restore();
			return;
		}
		if (!hasParent())
		{
			return;
		}
		if (_minimized)
		{
			_minimized = false;
			_applyMinimizedState();
		}
		_restoredGeometry = geometry();
		_maximized = true;
		_menuBar.maximizeButton().setIconSpriteID(_restoreSpriteID);
		_setGeometryWithoutConstraints(_maximizedGeometry());
	}

	void IInterfaceWindow::close()
	{
		deactivate();
	}
	IInterfaceWindow::ResizeContract IInterfaceWindow::subscribeToResize(ResizeCallback callback)
	{
		return _resizeProvider.subscribe(std::move(callback));
	}
	IInterfaceWindow::CloseContract IInterfaceWindow::subscribeToClose(CloseCallback callback)
	{
		return _menuBar.closeButton().subscribeToClick(std::move(callback));
	}
	bool IInterfaceWindow::isMinimized() const noexcept
	{
		return _minimized;
	}
	bool IInterfaceWindow::isMaximized() const noexcept
	{
		return _maximized;
	}
	bool IInterfaceWindow::isMoving() const noexcept
	{
		return _moving;
	}
	Widget *IInterfaceWindow::content() noexcept
	{
		return _content;
	}
	const Widget *IInterfaceWindow::content() const noexcept
	{
		return _content;
	}
	const std::optional<IInterfaceWindow::Padding> &IInterfaceWindow::contentPadding() const noexcept
	{
		return _contentPadding;
	}
	const Vector2 &IInterfaceWindow::minimumContentSize() const noexcept
	{
		return _minimumContentSize;
	}
	unsigned int IInterfaceWindow::menuHeight() const noexcept
	{
		return _menuHeight;
	}
	Panel &IInterfaceWindow::normalBackground() noexcept
	{
		return _normalBackground;
	}
	const Panel &IInterfaceWindow::normalBackground() const noexcept
	{
		return _normalBackground;
	}
	Panel &IInterfaceWindow::minimizedBackground() noexcept
	{
		return _minimizedBackground;
	}
	const Panel &IInterfaceWindow::minimizedBackground() const noexcept
	{
		return _minimizedBackground;
	}
	IInterfaceWindow::MenuBar &IInterfaceWindow::menuBar() noexcept
	{
		return _menuBar;
	}
	const IInterfaceWindow::MenuBar &IInterfaceWindow::menuBar() const noexcept
	{
		return _menuBar;
	}
}
