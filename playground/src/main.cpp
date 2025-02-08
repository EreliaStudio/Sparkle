#include "playground.hpp"

class MapEditorHUD : public spk::Widget
{
private:
	class InterfaceContent : public spk::AbstractInterfaceWindow::Content
	{
	private:
		spk::TextLabel _layerTextlabel;
		spk::PushButton _layerUpButton;
		spk::PushButton::Contract _layerUpButtonContract;
		spk::TextEdit _layerValueLabel;
		spk::PushButton _layerDownButton;
		spk::PushButton::Contract _layerDownButtonContract;

		spk::ObservableValue<int> _value;
		spk::ObservableValue<int>::Contract _onValueEditionContract;
		int _step = 1;

		spk::Font::Size defaultFontSize()
		{
			return {16, 0};
		}

		void _onGeometryChange() override
		{
			spk::Font::Size fontSize = defaultFontSize();

			_layerTextlabel.setTextSize(fontSize);
			spk::Vector2UInt minimalTextLabelSize = _layerTextlabel.computeExpectedTextSize(fontSize) + _layerTextlabel.cornerSize() * 2;
			spk::Vector2UInt buttonSize = std::max(16u, minimalTextLabelSize.y);

			_layerTextlabel.setGeometry({0, 0}, minimalTextLabelSize);

			_layerUpButton.setTextSize(fontSize);
			_layerUpButton.setGeometry({_layerTextlabel.geometry().size.x + 3, 0}, buttonSize);
			
			_layerValueLabel.setTextSize(fontSize);
			_layerValueLabel.setGeometry({_layerTextlabel.geometry().size.x + 3 + _layerUpButton.geometry().size.x + 3, 0}, buttonSize);
			
			_layerDownButton.setTextSize(fontSize);
			_layerDownButton.setGeometry({_layerTextlabel.geometry().size.x + 3 + _layerUpButton.geometry().size.x + 3 + _layerValueLabel.geometry().size.x + 3, 0}, buttonSize);
		}

	public:
		InterfaceContent(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::AbstractInterfaceWindow::Content(p_name, p_parent),
			_layerTextlabel(L"Layer text label", this),
			_layerUpButton(L"Layer up button", this),
			_layerValueLabel(L"Layer value entry", this),
			_layerDownButton(L"Layer down button", this),
			_value(0),
			_step(1),
			_onValueEditionContract(_value.subscribe([&](){_layerValueLabel.setText(std::to_wstring(_value.get()));}))
		{
			DEBUG_LINE();
			_layerTextlabel.setText(L"Layer :");
			_layerTextlabel.setTextAlignment(
				spk::HorizontalAlignment::Right,
				spk::VerticalAlignment::Centered
				);
			_layerTextlabel.setCornerSize(2);
			_layerTextlabel.activate();
			DEBUG_LINE();

			_layerUpButton.setText(L"+");
			_layerUpButton.setTextAlignment(
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered
				);
			_layerUpButtonContract = _layerUpButton.subscribe([&](){
				_value += _step;
			});
			_layerUpButton.setCornerSize(2);
			_layerUpButton.activate();
			DEBUG_LINE();

			_layerValueLabel.setText(L"0");
			_layerValueLabel.setTextAlignment(
				spk::HorizontalAlignment::Left,
				spk::VerticalAlignment::Centered
				);
			_layerValueLabel.setCornerSize(2);
			_layerValueLabel.activate();
			DEBUG_LINE();

			_layerDownButton.setText(L"-");
			_layerDownButton.setTextAlignment(
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered
				);
			_layerDownButtonContract = _layerDownButton.subscribe([&](){
				_value -= _step;
			});
			_layerDownButton.setCornerSize(2);
			//_layerDownButton.activate();
			DEBUG_LINE();
		}

		spk::Vector2UInt minimalSize()
		{
			spk::Vector2UInt titleLabelSize = _layerTextlabel.computeExpectedTextSize(defaultFontSize()) + _layerTextlabel.cornerSize() * 2;
			spk::Vector2UInt buttonSize = 16;

			spk::Vector2UInt result = {
				titleLabelSize.x + 3 + buttonSize.x + 3 + buttonSize.x + 3 + buttonSize.x,
				std::max(titleLabelSize.y, buttonSize.y) + 5 + 20
			};

			return (result);
		}
	};

	spk::InterfaceWindow<InterfaceContent> interfaceWindow;

	void _onGeometryChange() override
	{
		spk::Vector2UInt childSize = interfaceWindow.minimalSize();
		interfaceWindow.setGeometry((geometry().size - childSize) / 2 , childSize);
	}
	void _onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		switch (p_event.type)
		{
			case spk::KeyboardEvent::Type::Press:
			{
				if (p_event.key == spk::Keyboard::F12)
				{
					if (interfaceWindow.parent() == nullptr)
					{
						addChild(&interfaceWindow);
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
		interfaceWindow(L"Editor window", this)
	{
		interfaceWindow.setMinimumSize({150, 150});
		interfaceWindow.setMenuHeight(25);
		interfaceWindow.setLayer(10);
		interfaceWindow.activate();

		_quitContract = interfaceWindow.subscribeTo(spk::AbstractInterfaceWindow::Event::Close, [&](){removeChild(&interfaceWindow);});
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