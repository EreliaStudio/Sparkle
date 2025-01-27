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

		_topAnchorArea = {geometry().anchor, {geometry().size.x, 20}};
		_leftAnchorArea = {geometry().anchor, {20, geometry().size.y}};
		_rightAnchorArea = {{geometry().anchor.x + geometry().size.x - 20, geometry().anchor.y}, {20, geometry().size.y}};
		_downAnchorArea = {{geometry().anchor.x, geometry().anchor.y + geometry().size.y - 20}, {geometry().size.x, 20}};
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
				p_event.consume();
			}
			else if (_isTopResizing == true || _isDownResizing == true ||
					 _isLeftResizing == true || _isRightResizing == true)
			{
				spk::Vector2Int delta = p_event.mouse->position - _positionDelta;
				spk::Geometry2D newGeometry = _baseGeometry;

				if (_isTopResizing == true)
				{
					if (delta.y > static_cast<int>(_baseGeometry.height - _minimumSize.y))
						delta.y = static_cast<int>(_baseGeometry.height - _minimumSize.y);

					newGeometry.anchor.y += delta.y;
					newGeometry.size.y = _baseGeometry.anchor.y + _baseGeometry.size.y - newGeometry.anchor.y;
				}
				else if (_isDownResizing == true)
				{
					if (static_cast<int>(newGeometry.size.y) + delta.y >= static_cast<int>(_minimumSize.y))
						newGeometry.size.y += delta.y;
					else
						newGeometry.size.y = _minimumSize.y;
				}

				if (_isLeftResizing == true)
				{
					if (delta.x > static_cast<int>(_baseGeometry.width - _minimumSize.x))
						delta.x = static_cast<int>(_baseGeometry.width - _minimumSize.x);

					newGeometry.anchor.x += delta.x;
					newGeometry.size.x = _baseGeometry.anchor.x + _baseGeometry.size.x - newGeometry.anchor.x;
				}
				else if (_isRightResizing == true)
				{
					if (static_cast<int>(newGeometry.size.x) + delta.x >= static_cast<int>(_minimumSize.x))
						newGeometry.size.x += delta.x;
					else
						newGeometry.size.x = _minimumSize.x;
				}

				if (newGeometry != geometry())
				{
					setGeometry(newGeometry);
					p_event.requestPaint();
					p_event.consume();
				}
			}
			break;
		}
		case spk::MouseEvent::Type::Press:
		{
			if (viewport().geometry().contains(p_event.mouse->position))
				p_event.consume();

			if (p_event.button == spk::Mouse::Button::Left)
			{
				if (_menuBar._titleLabel.viewport().geometry().contains(p_event.mouse->position))
				{
					_isMoving = true;
					_positionDelta = p_event.mouse->position - geometry().anchor;
				}
				else
				{
					if (_topAnchorArea.contains(p_event.mouse->position))
					{
						_isTopResizing = true;
					}
					else if (_downAnchorArea.contains(p_event.mouse->position))
					{
						_isDownResizing = true;
					}

					if (_leftAnchorArea.contains(p_event.mouse->position))
					{
						_isLeftResizing = true;
					}
					else if (_rightAnchorArea.contains(p_event.mouse->position))
					{
						_isRightResizing = true;
					}	

					if (_isTopResizing == true || _isDownResizing == true ||
						_isLeftResizing == true || _isRightResizing == true)
					{
						_positionDelta = p_event.mouse->position;
						_baseGeometry = geometry();
					}
				}
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				_isMoving = false;
				_isTopResizing = false;
				_isLeftResizing = false;
				_isRightResizing = false;
				_isDownResizing = false;
				if (_menuBar._titleLabel.viewport().geometry().contains(p_event.mouse->position))
				{
					p_event.consume();
				}
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
		_menuBar.setLayer(3);
		_menuBar.activate();

		_backgroundFrame.setLayer(1);
		_backgroundFrame.activate();

		_minimizedBackgroundFrame.setLayer(1);
		_minimizedBackgroundFrame.deactivate();

		_contentFrame.setLayer(2);
		_contentFrame.setSpriteSheet(nullptr);
		_contentFrame.activate();

		_minimizeContract = _menuBar._minimizeButton.subscribe([&](){minimize();});
		_maximizeContract = _menuBar._maximizeButton.subscribe([&](){maximize();});
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

	void InterfaceWindow::setMinimumSize(const spk::Vector2UInt& p_minimumSize)
	{
		_minimumSize = p_minimumSize;
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