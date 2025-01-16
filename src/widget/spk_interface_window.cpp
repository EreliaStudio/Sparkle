#include "widget/spk_interface_window.hpp"
#include "spk_generated_resources.hpp"

namespace spk
{
	spk::SpriteSheet InterfaceWindow::_defaultIconset = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultIconset.png"), spk::Vector2Int(10, 10), SpriteSheet::Filtering::Linear);

	spk::SpriteSheet InterfaceWindow::_defaultNineSlice_Light = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Light.png"), spk::Vector2UInt(3, 3), SpriteSheet::Filtering::Linear);

	spk::SpriteSheet InterfaceWindow::_defaultNineSlice_Dark = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Dark.png"), spk::Vector2UInt(3, 3), SpriteSheet::Filtering::Linear);

	spk::SpriteSheet InterfaceWindow::_defaultNineSlice_Darker = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Darker.png"), spk::Vector2UInt(3, 3), SpriteSheet::Filtering::Linear);

	void InterfaceWindow::MenuBar::_onGeometryChange()
	{
		spk::Vector2Int buttonSize = geometry().size.y - 6;
		spk::Vector2Int controlButtonSize = spk::Vector2Int(buttonSize.x * 3, buttonSize.y);

		size_t textSize = buttonSize.y;

		_titleLabel.setTextSize({textSize - _titleLabel.cornerSize().y * 2, 1});

		spk::Vector2Int anchor = 3;

		_titleLabel.setGeometry 	(anchor, {geometry().size.x - controlButtonSize.x - 16, buttonSize.y});
		anchor.x += _titleLabel.geometry().size.x + 3;

		_minimizeButton.setGeometry	(anchor, buttonSize);
		anchor.x += _minimizeButton.geometry().size.x + 3;

		_maximizeButton.setGeometry (anchor, buttonSize);
		anchor.x += _maximizeButton.geometry().size.x + 3;

		_closeButton.setGeometry	(anchor, buttonSize);
	}

	InterfaceWindow::MenuBar::MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name + L" - Menu bar", p_parent),
		_titleLabel(p_name + L" - Title label", this),
		_closeButton(p_name + L" - Close button", this),
		_minimizeButton(p_name + L" - Minimize button", this),
		_maximizeButton(p_name + L" - Maximize button", this)
	{
		_titleLabel.setCornerSize(2);
		_titleLabel.setSpriteSheet(nullptr);
		_titleLabel.setText(p_name);
		_titleLabel.activate();

		_closeButton.setIconset(&_defaultIconset);
		_closeButton.setSprite(_defaultIconset.sprite(0));
		_closeButton.setCornerSize(4);
		_closeButton.activate();

		_minimizeButton.setIconset(&_defaultIconset);
		_minimizeButton.setSprite(_defaultIconset.sprite(3));
		_minimizeButton.setCornerSize(4);
		_minimizeButton.activate();

		_maximizeButton.setIconset(&_defaultIconset);
		_maximizeButton.setSprite(_defaultIconset.sprite(1));
		_maximizeButton.setCornerSize(4);
		_maximizeButton.activate();
	}

	void InterfaceWindow::_onGeometryChange()
	{
		spk::Vector2Int menuSize = spk::Vector2Int(geometry().size.x, _menuHeight);
		spk::Vector2Int frameSize = spk::Vector2Int(geometry().size.x, geometry().size.y - menuSize.y);

		_backgroundFrame.setGeometry(0, geometry().size);
		_minimizedBackgroundFrame.setGeometry(0, menuSize);
		_menuBar.setGeometry({0, 0}, menuSize);
		_contentFrame.setGeometry({0, menuSize.y}, frameSize);
	}

	void InterfaceWindow::_onMouseEvent(spk::MouseEvent& p_event)
	{
		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Motion:
		{
			if (_isMoving == true)
			{
				place(p_event.mouse->position - _positionDelta);
				p_event.requestPaint();
			}
			break;
		}
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button == spk::Mouse::Button::Left &&
				_menuBar._titleLabel.viewport().geometry().contains(p_event.mouse->position))
			{
				_isMoving = true;
				_positionDelta = p_event.mouse->position - geometry().anchor;
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				_isMoving = false;
			}
			break;
		}
		}		
	}

	InterfaceWindow::InterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent),
		_menuBar(p_name, this),
		_contentFrame(p_name + L" - Content frame", this),
		_backgroundFrame(p_name + L" - Background frame", this),
		_minimizedBackgroundFrame(p_name + L" - Background frame (Minimized)", this)
	{
		_menuBar.setLayer(2);
		_menuBar.activate();

		_backgroundFrame.setLayer(0);
		_backgroundFrame.activate();

		_minimizedBackgroundFrame.setLayer(0);
		_minimizedBackgroundFrame.deactivate();

		_contentFrame.setLayer(1);
		_contentFrame.setSpriteSheet(nullptr);
		_contentFrame.activate();

		_minimizeContract = _menuBar._minimizeButton.subscribe([&](){minimize();});
		_maximizeContract = _menuBar._maximizeButton.subscribe([&](){maximize();});
		_closeContract = _menuBar._closeButton.subscribe([&](){close();});
	}

	spk::SafePointer<spk::Widget> InterfaceWindow::content()
	{
		return (&_contentFrame);
	}

	void InterfaceWindow::setMenuHeight(const float& p_menuHeight)
	{
		_menuHeight = p_menuHeight;
		requireGeometryUpdate();
	}

	void InterfaceWindow::minimize()
	{
		if (_isMaximized == true)
			maximize();

		if (_backgroundFrame.isActive() == true)
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
	
	void InterfaceWindow::maximize()
	{
		_backgroundFrame.activate();
		
		if (_isMaximized == false)
		{
			_menuBar._maximizeButton.setSprite(_defaultIconset.sprite(2));
			_previousGeometry = geometry();
			setGeometry({0, 0}, parent()->geometry().size);
			_isMaximized = true;
		}
		else
		{
			_menuBar._maximizeButton.setSprite(_defaultIconset.sprite(1));
			setGeometry(_previousGeometry);
			_isMaximized = false;
		}
	}

	void InterfaceWindow::close()
	{
		parent()->removeChild(this);
	}

	spk::ContractProvider::Contract InterfaceWindow::subscribeTo(const InterfaceWindow::Event& p_event, const spk::ContractProvider::Job& p_job)
	{
		switch (p_event)
		{
			case InterfaceWindow::Minimize:
			{
				return (_menuBar._minimizeButton.subscribe(p_job));
			}
			case InterfaceWindow::Maximize:
			{
				return (_menuBar._maximizeButton.subscribe(p_job));
			}
			case InterfaceWindow::Close:
			{
				return (_menuBar._closeButton.subscribe(p_job));
			}
		}
		throw std::runtime_error("Unknow interface window event");
	}
}