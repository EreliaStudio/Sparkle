#include "playground.hpp"

class NodeSelectorWidget : public spk::Widget
{
private:
	static inline const spk::Vector2UInt _nbElement = {10, 20};
	static inline const spk::Vector2UInt _elementSize = {32, 32};

	spk::ColorRenderer _backgroundRenderer;

	void _onGeometryChange() override
	{
		_backgroundRenderer.clear();

		for (size_t i = 0; i < _nbElement.x; i++)
		{
			for (size_t j = 0; j < _nbElement.y; j++)
			{
				_backgroundRenderer.prepareSquare(spk::Geometry2D(
					geometry().anchor + ((_elementSize + 5) * spk::Vector2UInt(i, j)),
					_elementSize
				), layer());
			}
		}
		
		_backgroundRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_backgroundRenderer.render();
	}

public:
	NodeSelectorWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_backgroundRenderer.setColor(spk::Color(220, 50, 50));
	}

	spk::Vector2UInt requiredSize() const
	{
		return (_nbElement * _elementSize + 5 * (_nbElement - 1));
	}
};

class MapEditorHUD : public spk::Widget
{
private:
	class InterfaceContent : public spk::Widget
	{
	private:
		spk::PushButton _layerSelectionPushButtons[5];
		spk::ScrollableWidget<NodeSelectorWidget> _nodeSelector;

		spk::Font::Size defaultFontSize()
		{
			return {16, 0};
		}

		void _onGeometryChange() override
		{
			float space = 5;
			spk::Vector2 nbLayerButtons = {5, 1};
			spk::Vector2 layerSelectionButtonSize = {32, 32};

			spk::Vector2 anchorOffset = {
				(geometry().size.x - (layerSelectionButtonSize.x * 5)) / 4, 
				0
			};

			for (size_t i = 0; i < 5; i++)
			{
				_layerSelectionPushButtons[i].setGeometry(
					spk::Vector2((layerSelectionButtonSize.x + anchorOffset.x) * i, 0),
					layerSelectionButtonSize
				);
			}

			_nodeSelector.setGeometry({0, 5 + layerSelectionButtonSize.y}, geometry().size - spk::Vector2UInt(0, layerSelectionButtonSize.y + 5));
		}

	public:
		InterfaceContent(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_nodeSelector(L"NodeSelector", this),
			_layerSelectionPushButtons{
				spk::PushButton(L"Layer selection button A", this),
				spk::PushButton(L"Layer selection button B", this),
				spk::PushButton(L"Layer selection button C", this),
				spk::PushButton(L"Layer selection button D", this),
				spk::PushButton(L"Layer selection button E", this)
			}
		{
			for (size_t i = 0; i < 5; i++)
			{
				spk::SafePointer<spk::SpriteSheet> iconset = TextureManager::instance()->spriteSheet(L"iconset");

				_layerSelectionPushButtons[i].setCornerSize(2);
				_layerSelectionPushButtons[i].setIconset(iconset);
				_layerSelectionPushButtons[i].setSprite(iconset->sprite(i));
				_layerSelectionPushButtons[i].activate();
			}

			_nodeSelector.setContentSize(_nodeSelector.content()->requiredSize());
			_nodeSelector.activate();
		}

		spk::Vector2UInt minimalSize()
		{
			float space = 5.0f;
			spk::Vector2 nbLayerButtons = {5, 1};
			spk::Vector2 layerSelectionbutton = {32, 32};

			spk::Vector2 layerLineSize = {
				layerSelectionbutton.x * nbLayerButtons.x + space * (nbLayerButtons.x - 1),
				layerSelectionbutton.y};	

			spk::Vector2 nodeItemSize = {32, 32};
			spk::Vector2 nodeRenderedOnSelector = {8, 4};
			spk::Vector2 nodeSelectorSize = {
				nodeItemSize.x * nodeRenderedOnSelector.x + space * (nodeRenderedOnSelector.x - 1),
				nodeItemSize.y * nodeRenderedOnSelector.y + space * (nodeRenderedOnSelector.y - 1)
			};

			spk::Vector2UInt result = {
					std::max(layerLineSize.x, nodeSelectorSize.x),
					layerLineSize.y + space + nodeSelectorSize.y
				};

			return (result);
		}
	};

	spk::InterfaceWindow<InterfaceContent> _interfaceWindow;

	void _onGeometryChange() override
	{
		spk::Vector2UInt childSize = _interfaceWindow.content()->minimalSize();
		_interfaceWindow.setGeometry((geometry().size - childSize) / 2 , childSize);
	}

	void _onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		switch (p_event.type)
		{
			case spk::KeyboardEvent::Type::Press:
			{
				if (p_event.key == spk::Keyboard::F12)
				{
					if (_interfaceWindow.parent() == nullptr)
					{
						addChild(&_interfaceWindow);
					}
				}
				
				break;
			}
		}
	}

	spk::ContractProvider::Contract _quitContract;

public:
	MapEditorHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_interfaceWindow(L"Editor window", this)
	{
		spk::Vector2UInt minimalSize = _interfaceWindow.content()->minimalSize();
		_interfaceWindow.setMinimumContentSize(minimalSize);
		_interfaceWindow.setMenuHeight(25);
		_interfaceWindow.setLayer(10);
		_interfaceWindow.activate();

		_quitContract = _interfaceWindow.subscribeTo(spk::IInterfaceWindow::Event::Close, [&](){removeChild(&_interfaceWindow);});
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

	MapEditorHUD mapEditorWidget = MapEditorHUD(L"Editor window", win->widget());
	mapEditorWidget.setGeometry(win->geometry());
	mapEditorWidget.activate();

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setLayer(0);
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();
	
	return (app.run());
}