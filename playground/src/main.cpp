#include "playground.hpp"

class PushButton : public spk::PushButton
{
private:

public:
	PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent = nullptr) : 
		spk::PushButton(p_name, p_parent)
	{
		setCornerSize(2);
		setTextSize({14, 2});
		setTextColor(spk::Color::white, spk::Color::black);
		setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
		setPressedOffset(2);
		setSpriteSheet(TextureManager::instance()->spriteSheet(L"defaultNineSlice"));
		setFont(TextureManager::instance()->font(L"defaultFont"));
	}
};

class TextLabel : public spk::TextLabel
{
private:

public:
	TextLabel(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent = nullptr) : 
		spk::TextLabel(p_name, p_parent)
	{
		setCornerSize(2);
		setTextSize({14, 2});
		setTextColor(spk::Color::white, spk::Color::black);
		setTextAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		setSpriteSheet(TextureManager::instance()->spriteSheet(L"defaultNineSlice"));
		setFont(TextureManager::instance()->font(L"defaultFont"));
	}
};

class Frame : public spk::Frame
{
private:

public:
	Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent = nullptr) : 
		spk::Frame(p_name, p_parent)
	{
		setCornerSize(2);
		setSpriteSheet(TextureManager::instance()->spriteSheet(L"defaultNineSlice"));
	}
};

class InterfaceWindow : public spk::Widget
{
private:
	class MenuBar : public spk::Widget
	{
		friend class InterfaceWindow;

	private:
		TextLabel _titleLabel;
		PushButton _reduceButton;
		PushButton _maximizeButton;
		PushButton _closeButton;

		void _onGeometryChange()
		{
			spk::Vector2Int controlButtonSize = spk::Vector2Int(geometry().size.y * 3 + 2, geometry().size.y);
			spk::Vector2Int buttonSize = spk::Vector2Int((controlButtonSize.x - 2) / 3, geometry().size.y);

			_titleLabel.setGeometry 	({0, 0}, {geometry().size.x - controlButtonSize.x - 1, buttonSize.y});
			_reduceButton.setGeometry	({geometry().size.x - controlButtonSize.x + (buttonSize.x + 1) * 0, 0}, buttonSize);
			_maximizeButton.setGeometry ({geometry().size.x - controlButtonSize.x + (buttonSize.x + 1) * 1, 0}, buttonSize);
			_closeButton.setGeometry	({geometry().size.x - controlButtonSize.x + (buttonSize.x + 1) * 2, 0}, buttonSize);
		}

	public:
		MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
			spk::Widget(p_name + L" - Menu bar", p_parent),
			_titleLabel(p_name + L" - Title label", this),
			_closeButton(p_name + L" - Close button", this),
			_reduceButton(p_name + L" - Reduce button", this),
			_maximizeButton(p_name + L" - Maximize button", this)
		{
			_titleLabel.setText(p_name);
			_titleLabel.activate();

			_closeButton.setText(L"X");
			_closeButton.activate();

			_reduceButton.setText(L"_");
			_reduceButton.activate();

			_maximizeButton.setText(L"M");
			_maximizeButton.activate();
		}
	};

	MenuBar _menuBar;
	Frame _backgroundFrame;

	void _onGeometryChange() override
	{
		spk::Vector2Int menuSize = spk::Vector2Int(geometry().size.x, 20);
		spk::Vector2Int frameSize = spk::Vector2Int(geometry().size.x, geometry().size.y - menuSize.y);

		_menuBar.setGeometry({0, 0}, menuSize);
		_backgroundFrame.setGeometry({0, menuSize.y}, frameSize);
	}

	bool _isMoving = false;
	spk::Vector2Int _positionDelta;

	void _onMouseEvent(spk::MouseEvent& p_event) override
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

	spk::ContractProvider::Contract _closeContract;
	spk::ContractProvider::Contract _reduceContract;

	void reduceEffect()
	{
		if (_isMaximized == true)
			maximizeEffect();

		if (_backgroundFrame.isActive() == true)
			_backgroundFrame.deactivate();
		else
			_backgroundFrame.activate();
	}

	spk::ContractProvider::Contract _maximizeContract;
	bool _isMaximized = false;
	spk::Geometry2D _previousGeometry;

	void maximizeEffect()
	{
		_backgroundFrame.activate();
		
		if (_isMaximized == false)
		{
			_previousGeometry = geometry();
			setGeometry({0, 0}, parent()->geometry().size);
			_isMaximized = true;
		}
		else
		{
			setGeometry(_previousGeometry);
			_isMaximized = false;
		}
	}

public:
	InterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent),
		_menuBar(p_name, this),
		_backgroundFrame(p_name + L" - Background frame", this)
	{
		_menuBar.activate();
		_backgroundFrame.activate();

		_reduceContract = _menuBar._reduceButton.subscribe([&](){reduceEffect();});
		_maximizeContract = _menuBar._maximizeButton.subscribe([&](){maximizeEffect();});
		_closeContract = _menuBar._closeButton.subscribe([&](){parent()->removeChild(this);});
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	TextureManager::instanciate();
	
	NodeMap nodeMap;

	spk::GameEngine engine;

	spk::Entity worldManager = spk::Entity(L"World manager");
	auto& worldManagerComp = worldManager.addComponent<WorldManager>(L"World manager component");
	worldManager.addComponent<ControlMapper>(L"Control mapper component");

	spk::Entity player = spk::Entity(L"Player");
	player.transform().place(spk::Vector3(0, 0, 0));
	player.addComponent<PlayerController>(L"Player controler component");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, 20));
	camera.transform().lookAt(player.transform().position());

	CameraManager& cameraComp = camera.addComponent<CameraManager>(L"Main camera");

	worldManagerComp.setCamera(&camera);

	engine.addEntity(&worldManager);
	engine.addEntity(&player);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setLayer(0);
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	InterfaceWindow window = InterfaceWindow(L"Editor inventory", win->widget());
	window.setGeometry({-100, 100}, win->geometry().size / 2);
	window.activate();

	return (app.run());
}