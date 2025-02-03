#include "playground.hpp"

class TextEntry : public spk::Widget
{
private:
	std::wstring _text;
	std::wstring _placeholder;

	spk::Vector2Int _cornerSize = {2, 2};
	spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Left;
	spk::VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Centered;

	spk::NineSliceRenderer _backgroundRenderer;
	spk::FontRenderer _fontRenderer;

	void _onGeometryChange() override
	{
		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(geometry(), layer(), _cornerSize);
		_backgroundRenderer.validate();

		_fontRenderer.clear();
		spk::Vector2Int textAnchor = _fontRenderer.computeTextAnchor(geometry().shrink(_cornerSize), text(), _horizontalAlignment, _verticalAlignment);
		_fontRenderer.prepare(text(), textAnchor, layer() + 0.01f);
		_fontRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_fontRenderer.render();
	}

public:
	TextEntry(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
	}

	void setText(const std::wstring& p_text)
	{
		_text = p_text;
	}

	void setPlaceholder(const std::wstring& p_placeholder)
	{
		_placeholder = p_placeholder;
	}

	std::wstring text()
	{
		if (_text.empty() == true)
		{
			return (_placeholder);
		}
		return (_text);
	}
};

class MapEditorHUD : public spk::Widget
{
private:
	class InterfaceContent : public spk::AbstractInterfaceWindow::Content
	{
	private:
		spk::TextLabel _layerTextlabel;
		spk::PushButton _layerUpButton;
		spk::TextLabel _layerValueLabel;
		spk::PushButton _layerDownButton;

		spk::Font::Size defaultFontSize()
		{
			return {16, 1};
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
			_layerValueLabel(L"Layer value label", this),
			_layerDownButton(L"Layer down button", this)
		{
			_layerTextlabel.setText(L"Layer :");
			_layerTextlabel.setTextAlignment(
				spk::HorizontalAlignment::Right,
				spk::VerticalAlignment::Centered
				);
			_layerTextlabel.setCornerSize(2);
			_layerTextlabel.activate();

			_layerUpButton.setText(L"+");
			_layerUpButton.setTextAlignment(
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered
				);
			_layerUpButton.setCornerSize(2);
			_layerUpButton.activate();

			_layerValueLabel.setText(L"0");
			_layerValueLabel.setTextAlignment(
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered
				);
			_layerValueLabel.setCornerSize(2);
			_layerValueLabel.activate();

			_layerDownButton.setText(L"-");
			_layerDownButton.setTextAlignment(
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered
				);
			_layerDownButton.setCornerSize(2);
			_layerDownButton.activate();
		}

		spk::Vector2UInt minimalSize()
		{
			spk::Vector2UInt titleLabelSize = _layerTextlabel.computeExpectedTextSize(defaultFontSize()) + _layerTextlabel.cornerSize() * 2;
			spk::Vector2UInt buttonSize = 16;

			spk::Vector2UInt result = {
				titleLabelSize.x + 3 + buttonSize.x + 3 + buttonSize.x + 3 + buttonSize.x,
				std::max(titleLabelSize.y, buttonSize.y)
			};

			return (result);
		}
	};

	spk::InterfaceWindow<InterfaceContent> interfaceWindow;

	void _onGeometryChange() override
	{
		interfaceWindow.setGeometry({0, 0}, {25, 50});
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