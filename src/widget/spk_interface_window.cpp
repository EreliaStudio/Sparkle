#include "widget/spk_interface_window.hpp"
#include "spk_generated_resources.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	spk::SpriteSheet IInterfaceWindow::_defaultNineSlice_Light = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Light.png"), spk::Vector2UInt(3, 3), spk::SpriteSheet::Filtering::Linear);

	spk::SpriteSheet IInterfaceWindow::_defaultNineSlice_Dark = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Dark.png"), spk::Vector2UInt(3, 3), spk::SpriteSheet::Filtering::Linear);

	spk::SpriteSheet IInterfaceWindow::_defaultNineSlice_Darker = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Darker.png"), spk::Vector2UInt(3, 3), spk::SpriteSheet::Filtering::Linear);

	spk::Font::Size IInterfaceWindow::MenuBar::_computeFontSize(const float &p_menuHeight)
	{
		spk::Vector2Int buttonSize = _computeControlButtonSize(p_menuHeight);
		float outlineSize = 0;
		return (spk::Font::Size(buttonSize.y - outlineSize * 2, outlineSize));
	}

	spk::Vector2Int IInterfaceWindow::MenuBar::_computeControlButtonSize(const float &p_menuHeight)
	{
		if (_titleLabel.cornerSize().y * 4 > p_menuHeight)
		{
			return (p_menuHeight);
		}
		return (p_menuHeight - _titleLabel.cornerSize().y * 4);
	}

	void IInterfaceWindow::MenuBar::_onGeometryChange()
	{
		float space = 3.f;

		spk::Vector2Int buttonSize = _computeControlButtonSize(geometry().size.y);

		int activeCount = (_minimizeButton.isActive() ? 1 : 0) + (_maximizeButton.isActive() ? 1 : 0) + (_closeButton.isActive() ? 1 : 0);
		int usedWidth = activeCount * buttonSize.x + (activeCount + 1) * (int)space + _titleLabel.cornerSize().x * 4;

		spk::Vector2Int titleLabelSize = {geometry().size.x - usedWidth, buttonSize.y};
		_titleLabel.setFontSize(_computeFontSize(geometry().size.y));

		spk::Vector2Int anchor = _titleLabel.cornerSize() * 2;
		_titleLabel.setGeometry(anchor, titleLabelSize);

		anchor.x += titleLabelSize.x + (int)space;

		auto placeButton = [&](spk::PushButton &p_button)
		{
			if (!p_button.isActive())
			{
				return;
			}
			p_button.setGeometry(anchor, buttonSize);
			anchor.x += buttonSize.x + (int)space;
		};

		placeButton(_minimizeButton);
		placeButton(_maximizeButton);
		placeButton(_closeButton);
	}

	spk::Vector2UInt IInterfaceWindow::MenuBar::_computeMinimalSize(const float &p_menuHeight)
	{
		spk::Vector2Int buttonSize = _computeControlButtonSize(p_menuHeight);
		spk::Font::Size fontSize = _computeFontSize(p_menuHeight);

		spk::Vector2Int titleLabelSize = _titleLabel.font()->computeStringSize(_titleLabel.text(), fontSize);

		return (spk::Vector2UInt(titleLabelSize.x + buttonSize.x * 3 + 3 * 4 + _titleLabel.cornerSize().x * 4, p_menuHeight));
	}

	void IInterfaceWindow::MenuBar::_activateMenuButton(const MenuBar::Button &p_button)
	{
		switch (p_button)
		{
		case MenuBar::Button::Minimize:
			_minimizeButton.activate();
			break;
		case MenuBar::Button::Maximize:
			_maximizeButton.activate();
			break;
		case MenuBar::Button::Close:
			_closeButton.activate();
			break;
		}
		requireGeometryUpdate();
	}

	void IInterfaceWindow::MenuBar::_deactivateMenuButton(const MenuBar::Button &p_button)
	{
		switch (p_button)
		{
		case MenuBar::Button::Minimize:
			_minimizeButton.deactivate();
			break;
		case MenuBar::Button::Maximize:
			_maximizeButton.deactivate();
			break;
		case MenuBar::Button::Close:
			_closeButton.deactivate();
			break;
		}
		requireGeometryUpdate();
	}

	IInterfaceWindow::MenuBar::MenuBar(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent) :
		spk::Widget(p_name + L" - Menu bar", p_parent),
		_titleLabel(p_name + L" - Title label", this),
		_closeButton(p_name + L" - Close button", this),
		_minimizeButton(p_name + L" - Minimize button", this),
		_maximizeButton(p_name + L" - Maximize button", this)
	{
		_titleLabel.setCornerSize(2);
		_titleLabel.setNineSlice(nullptr);
		_titleLabel.setText(p_name);
		_titleLabel.activate();

		_closeButton.setIconset(spk::Widget::defaultIconset());
		_closeButton.setIcon(spk::Widget::defaultIconset()->sprite(0));
		_closeButton.setCornerSize(4);
		_closeButton.activate();

		_minimizeButton.setIconset(spk::Widget::defaultIconset());
		_minimizeButton.setIcon(spk::Widget::defaultIconset()->sprite(3));
		_minimizeButton.setCornerSize(4);
		_minimizeButton.activate();

		_maximizeButton.setIconset(spk::Widget::defaultIconset());
		_maximizeButton.setIcon(spk::Widget::defaultIconset()->sprite(1));
		_maximizeButton.setCornerSize(4);
		_maximizeButton.activate();
	}

	IInterfaceWindow::IInterfaceWindow(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent) :
		ScalableWidget(p_name, p_parent),
		_menuBar(p_name, this),
		_backgroundFrame(p_name + L" - Background frame", this),
		_minimizedBackgroundFrame(p_name + L" - Background frame (Minimized)", this)
	{
		_menuBar.setLayer(3);
		_menuBar.activate();

		_backgroundFrame.setLayer(1);
		_backgroundFrame.activate();

		_minimizedBackgroundFrame.setLayer(1);
		_minimizedBackgroundFrame.deactivate();

		_minimizeContract = _menuBar._minimizeButton.subscribe(
			[&]()
			{
				minimize();
				requestPaint();
			});
		_maximizeContract = _menuBar._maximizeButton.subscribe(
			[&]()
			{
				maximize();
				requestPaint();
			});
	}

	void IInterfaceWindow::_onGeometryChange()
	{
		spk::Vector2Int menuSize = {geometry().size.x, static_cast<int>(_menuHeight)};

		spk::Vector2Int frameSize = {geometry().size.x - _backgroundFrame.cornerSize().x * 2,
									 geometry().size.y - menuSize.y - _backgroundFrame.cornerSize().y * 2};

		_backgroundFrame.setGeometry(0, geometry().size);
		_minimizedBackgroundFrame.setGeometry(0, menuSize);
		_menuBar.setGeometry({0, 0}, menuSize);

		if (_content != nullptr)
		{
			_content->setGeometry({_backgroundFrame.cornerSize().x, menuSize.y + _backgroundFrame.cornerSize().y}, frameSize);
		}
	}

	void IInterfaceWindow::_onMouseEvent(spk::MouseEvent &p_event)
	{
		ScalableWidget::_onMouseEvent(p_event);

		if (p_event.consumed() == true)
		{
			return;
		}

		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Motion:
		{
			if (_isMoving)
			{
				place(p_event.mouse->position - _positionDelta);
				p_event.requestPaint();
				p_event.consume();
			}
			break;
		}
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				if (_menuBar._titleLabel.viewport().geometry().contains(p_event.mouse->position))
				{
					_isMoving = true;
					_positionDelta = p_event.mouse->position - geometry().anchor;
					p_event.window->setCursor(L"Hand");
					p_event.consume();
				}
			}
			if (viewport().geometry().contains(p_event.mouse->position) == true)
			{
				p_event.consume();
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				_isMoving = false;
				p_event.window->setCursor(L"Arrow");
			}
			break;
		}
		}
	}

	spk::SafePointer<Widget> IInterfaceWindow::backgroundFrame()
	{
		return (&_backgroundFrame);
	}

	void IInterfaceWindow::setContent(spk::SafePointer<Widget> p_content)
	{
		_content = p_content;
		_backgroundFrame.addChild(_content);

		setMinimumContentSize(_content->minimalSize());
		setMaximumContentSize(_content->maximalSize());

		requireGeometryUpdate();
	}

	uint32_t safeAdd(uint32_t p_a, uint32_t p_b)
	{
		return p_a > std::numeric_limits<uint32_t>::max() - p_b ? std::numeric_limits<uint32_t>::max() : p_a + p_b;
	};

	void IInterfaceWindow::setMinimumContentSize(const spk::Vector2UInt &p_minimumContentSize)
	{
		spk::Vector2UInt menuBarSize = _menuBar._computeMinimalSize(_menuHeight);
		uint32_t extra = _backgroundFrame.cornerSize().y * 2;
		uint32_t x = std::max(safeAdd(p_minimumContentSize.x, extra), menuBarSize.x);
		uint32_t y = safeAdd(menuBarSize.y, safeAdd(p_minimumContentSize.y, extra));
		spk::ScalableWidget::setMinimumSize({x, y});
	}

	void IInterfaceWindow::setMaximumContentSize(const spk::Vector2UInt &p_maximumContentSize)
	{
		spk::Vector2UInt menuBarSize = _menuBar._computeMinimalSize(_menuHeight);
		uint32_t extra = _backgroundFrame.cornerSize().y * 2;
		uint32_t x = std::max(safeAdd(p_maximumContentSize.x, extra), menuBarSize.x);
		uint32_t y = safeAdd(menuBarSize.y, safeAdd(p_maximumContentSize.y, extra));
		spk::ScalableWidget::setMaximumSize({x, y});
	}

	void IInterfaceWindow::setMenuHeight(const float &p_menuHeight)
	{
		_menuHeight = p_menuHeight;

		setMinimumContentSize(_content->minimalSize());
		setMaximumContentSize(_content->maximalSize());

		requireGeometryUpdate();
	}

	void IInterfaceWindow::setCornerSize(const spk::Vector2Int &p_cornerSize)
	{
		_backgroundFrame.setCornerSize(p_cornerSize);

		setMinimumContentSize(_content->minimalSize());
		setMaximumContentSize(_content->maximalSize());

		requireGeometryUpdate();
	}

	void IInterfaceWindow::minimize()
	{
		if (_isMaximized)
		{
			maximize();
		}

		if (_backgroundFrame.isActive())
		{
			_minimizedBackgroundFrame.activate();
			_backgroundFrame.deactivate();
		}
		else
		{
			_minimizedBackgroundFrame.deactivate();
			_backgroundFrame.activate();
		}
	}

	void IInterfaceWindow::maximize()
	{
		_backgroundFrame.activate();

		if (!_isMaximized)
		{
			_menuBar._maximizeButton.setIcon(spk::Widget::defaultIconset()->sprite(2));
			_previousGeometry = geometry();
			setGeometry({0, 0}, parent()->geometry().size);
			_isMaximized = true;
		}
		else
		{
			_menuBar._maximizeButton.setIcon(spk::Widget::defaultIconset()->sprite(1));
			setGeometry(_previousGeometry);
			_isMaximized = false;
		}
	}

	void IInterfaceWindow::close()
	{
		deactivate();
	}

	void IInterfaceWindow::activateMenuButton(const MenuBar::Button &p_button)
	{
		_menuBar._activateMenuButton(p_button);
	}

	void IInterfaceWindow::deactivateMenuButton(const MenuBar::Button &p_button)
	{
		_menuBar._deactivateMenuButton(p_button);
	}

	spk::ContractProvider::Contract IInterfaceWindow::subscribeTo(const IInterfaceWindow::Event &p_event, const spk::ContractProvider::Job &p_job)
	{
		switch (p_event)
		{
		case IInterfaceWindow::Minimize:
			return _menuBar._minimizeButton.subscribe(p_job);
		case IInterfaceWindow::Maximize:
			return _menuBar._maximizeButton.subscribe(p_job);
		case IInterfaceWindow::Close:
			return _menuBar._closeButton.subscribe(p_job);
		}
		throw std::runtime_error("Unknown interface window event");
	}
}
