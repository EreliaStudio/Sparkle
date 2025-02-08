#include "widget/spk_interface_window.hpp"
#include "spk_generated_resources.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	spk::SpriteSheet AbstractInterfaceWindow::_defaultIconset =
		spk::SpriteSheet::fromRawData(
			SPARKLE_GET_RESOURCE("resources/textures/defaultIconset.png"),
			spk::Vector2Int(10, 10),
			spk::SpriteSheet::Filtering::Linear
		);

	spk::SpriteSheet AbstractInterfaceWindow::_defaultNineSlice_Light =
		spk::SpriteSheet::fromRawData(
			SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Light.png"),
			spk::Vector2UInt(3, 3),
			spk::SpriteSheet::Filtering::Linear
		);

	spk::SpriteSheet AbstractInterfaceWindow::_defaultNineSlice_Dark =
		spk::SpriteSheet::fromRawData(
			SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Dark.png"),
			spk::Vector2UInt(3, 3),
			spk::SpriteSheet::Filtering::Linear
		);

	spk::SpriteSheet AbstractInterfaceWindow::_defaultNineSlice_Darker =
		spk::SpriteSheet::fromRawData(
			SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Darker.png"),
			spk::Vector2UInt(3, 3),
			spk::SpriteSheet::Filtering::Linear
		);

	spk::Font::Size AbstractInterfaceWindow::MenuBar::_computeFontSize(const float& p_menuHeight)
	{
		spk::Vector2Int buttonSize = _computeControlButtonSize(p_menuHeight);
		float outlineSize = 0;
		return (spk::Font::Size(buttonSize.y - outlineSize * 2, outlineSize));
	}
	
	spk::Vector2Int AbstractInterfaceWindow::MenuBar::_computeControlButtonSize(const float& p_menuHeight)
	{
		if (_titleLabel.cornerSize().y * 4 > p_menuHeight)
			return (p_menuHeight);
		return (p_menuHeight - _titleLabel.cornerSize().y * 4);
	}
	
	void AbstractInterfaceWindow::MenuBar::_onGeometryChange()
	{
		float space = 3.0f;
		spk::Vector2Int buttonSize = _computeControlButtonSize(geometry().size.y);
		spk::Vector2Int titleLabelSize = { geometry().size.x - buttonSize.x * 3 - space * 4 - _titleLabel.cornerSize().x * 4, buttonSize.y };

		_titleLabel.setTextSize(_computeFontSize(geometry().size.y));

		spk::Vector2Int anchor = _titleLabel.cornerSize() * 2;

		_titleLabel.setGeometry(anchor, { titleLabelSize.x, titleLabelSize.y });
		anchor.x += titleLabelSize.x + space;

		_minimizeButton.setGeometry(anchor, buttonSize);
		anchor.x += buttonSize.x + space;

		_maximizeButton.setGeometry(anchor, buttonSize);
		anchor.x += buttonSize.x + space;

		_closeButton.setGeometry(anchor, buttonSize);
	}

	spk::Vector2UInt AbstractInterfaceWindow::MenuBar::_computeMinimalSize(const float& p_menuHeight)
	{
		spk::Vector2Int buttonSize = _computeControlButtonSize(p_menuHeight);
		spk::Font::Size fontSize = _computeFontSize(p_menuHeight);

		spk::Vector2Int titleLabelSize = _titleLabel.font()->computeStringSize(_titleLabel.text(), fontSize);

		return (spk::Vector2UInt(titleLabelSize.x + buttonSize.x * 3 + 3 * 4 + _titleLabel.cornerSize().x * 4, p_menuHeight ));
	}

	AbstractInterfaceWindow::MenuBar::MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
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

	AbstractInterfaceWindow::AbstractInterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		ScalableWidget(p_name, p_parent),
		_menuBar(p_name, this) ,
		_backgroundFrame(p_name + L" - Background frame", this),
		_minimizedBackgroundFrame(p_name + L" - Background frame (Minimized)", this)
	{
		_menuBar.setLayer(3);
		_menuBar.activate();

		_backgroundFrame.setLayer(1);
		_backgroundFrame.activate();

		_minimizedBackgroundFrame.setLayer(1);
		_minimizedBackgroundFrame.deactivate();

		_minimizeContract = _menuBar._minimizeButton.subscribe([&]() { minimize(); });
		_maximizeContract = _menuBar._maximizeButton.subscribe([&]() { maximize(); });
	}

	void AbstractInterfaceWindow::_onGeometryChange()
	{
		spk::Vector2Int menuSize = { geometry().size.x, static_cast<int>(_menuHeight) };
		spk::Vector2Int frameSize = {
			geometry().size.x - _backgroundFrame.cornerSize().x * 2,
			geometry().size.y - menuSize.y - _backgroundFrame.cornerSize().y * 2 };

		_backgroundFrame.setGeometry(0, geometry().size);
		_minimizedBackgroundFrame.setGeometry(0, menuSize);
		_menuBar.setGeometry({ 0, 0 }, menuSize);
		
		if (_content != nullptr)
			_content->setGeometry({ _backgroundFrame.cornerSize().x, menuSize.y + _backgroundFrame.cornerSize().y}, frameSize);
	}

	void AbstractInterfaceWindow::_onMouseEvent(spk::MouseEvent& p_event)
	{
		ScalableWidget::_onMouseEvent(p_event);
		
		if (p_event.consumed() == true)
			return;

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

	void AbstractInterfaceWindow::_computeMinimalSize()
	{
		setMinimumSize(minimalSize());
	}

	void AbstractInterfaceWindow::setContent(spk::SafePointer<Content> p_content)
	{
		_content = p_content;
		_backgroundFrame.addChild(_content);
		_computeMinimalSize();
		requireGeometryUpdate();
	}

	spk::Vector2UInt AbstractInterfaceWindow::minimalSize()
	{
		spk::Vector2UInt menuBarSize = _menuBar._computeMinimalSize(_menuHeight);

		spk::Vector2UInt contentSize = (_content == nullptr ? 0 : _content->minimalSize());

		return (spk::Vector2UInt(
				std::max(menuBarSize.x, contentSize.x),
				menuBarSize.y + contentSize.y + _backgroundFrame.cornerSize().y * 2
			));
	}

	void AbstractInterfaceWindow::setMenuHeight(const float& p_menuHeight)
	{
		_menuHeight = p_menuHeight;

		_computeMinimalSize();

		requireGeometryUpdate();
	}

	void AbstractInterfaceWindow::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_backgroundFrame.setCornerSize(p_cornerSize);

		requireGeometryUpdate();
	}

	void AbstractInterfaceWindow::minimize()
	{
		if (_isMaximized)
			maximize();

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
	
	void AbstractInterfaceWindow::maximize()
	{
		_backgroundFrame.activate();

		if (!_isMaximized)
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

	void AbstractInterfaceWindow::close()
	{
		deactivate();
	}

	spk::ContractProvider::Contract AbstractInterfaceWindow::subscribeTo(const AbstractInterfaceWindow::Event& p_event,
		const spk::ContractProvider::Job& p_job)
	{
		switch (p_event)
		{
		case AbstractInterfaceWindow::Minimize:
			return _menuBar._minimizeButton.subscribe(p_job);
		case AbstractInterfaceWindow::Maximize:
			return _menuBar._maximizeButton.subscribe(p_job);
		case AbstractInterfaceWindow::Close:
			return _menuBar._closeButton.subscribe(p_job);
		}
		throw std::runtime_error("Unknown interface window event");
	}
}
