#include "playground.hpp"


template <typename TType>
class Spinbox : public spk::Widget
{
private:
    spk::PushButton _upButton;
    spk::PushButton::Contract _upButtonContract;
    spk::TextEdit _valueEdit;
    spk::PushButton _downButton;
    spk::PushButton::Contract _downButtonContract;

	std::optional<TType> _minLimit;
	std::optional<TType> _maxLimit;
    spk::ObservableValue<TType> _value;
    spk::ObservableValue<TType>::Contract _onValueEditionContract;
    TType _step = static_cast<TType>(1);

    void _onGeometryChange() override
    {
        spk::Vector2UInt buttonSize = { std::max(16u, geometry().size.y), geometry().size.y };

        spk::Vector2UInt editSize = {
            geometry().size.x - (buttonSize.x * 2) - 6,
            geometry().size.y
        };

		_valueEdit.setTextSize({static_cast<size_t>((geometry().height - _valueEdit.cornerSize().y * 2)), 0});

        _valueEdit.setGeometry({ 0, 0 }, editSize);
        _downButton.setGeometry({editSize.x + 3, 0}, buttonSize);
        _upButton.setGeometry({editSize.x + 3 + buttonSize.x + 3, 0}, buttonSize);
    }

public:
    Spinbox(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_upButton(L"Increase", this),
		_valueEdit(L"Value Edit", this),
		_downButton(L"Decrease", this),
		_value(static_cast<TType>(0)),
        _onValueEditionContract(_value.subscribe([&]() { _valueEdit.setText(std::to_wstring(_value.get())); }))
    {
        _upButton.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
        _upButtonContract = _upButton.subscribe([&]() {
			if (_maxLimit.has_value() == false)
			{
				_value += _step;
			}
			else if (_value != _maxLimit.value())
			{
				_value = std::min(_value + _step, _maxLimit.value());
			}
			});
        _upButton.activate();

		_valueEdit.setPlaceholder(L"...");
        _valueEdit.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
        _valueEdit.activate();

        _downButton.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
        _downButtonContract = _downButton.subscribe([&](){
			if (_minLimit.has_value() == false)
			{
				_value -= _step;
			}
			else if (_value != _minLimit.value())
			{
				_value = std::max(_value - _step, _minLimit.value());
			}
			});
        _downButton.activate();

		_value.trigger();

		setIconSet(spk::Widget::defaultIconset());
    }

	void setIconSet(spk::SafePointer<spk::SpriteSheet> p_iconSet)
	{
		_upButton.setText(L"");
		_upButton.setIconset(p_iconSet);
		_upButton.setSprite(p_iconSet->sprite(4));

		_downButton.setIconset(p_iconSet);
		_downButton.setText(L"");
		_downButton.setSprite(p_iconSet->sprite(5));
	}

	void setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_upButton.setSpriteSheet(p_spriteSheet);
		_valueEdit.setSpriteSheet(p_spriteSheet);
		_downButton.setSpriteSheet(p_spriteSheet);
	}

	void setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_upButton.setCornerSize(p_cornerSize);
		_valueEdit.setCornerSize(p_cornerSize);
		_downButton.setCornerSize(p_cornerSize);
	}

    void setValue(TType p_value)
	{
		_value.set(p_value);
	}

    TType getValue() const
	{
		return _value.get();
	}

    void setStep(TType p_step)
	{
		_step = p_step;
	}

	void setMinimalLimit(TType p_minimalValue)
	{
		_minLimit = p_minimalValue;
	}

	void setMaximalLimit(TType p_maximalValue)
	{
		_maxLimit = p_maximalValue;
	}

	void setLimits(const TType& p_minimalValue, const TType& p_maximalValue)
	{
		setMinimalLimit(p_minimalValue);
		setMaximalLimit(p_maximalValue);
	}

	void removeLimits()
	{
		_minLimit.reset();
		_maxLimit.reset();
	}

	bool isEditEnable() const
	{
		return (_valueEdit.isEditEnable());
	}

	void enableEdit()
	{
		_valueEdit.disableEdit();
	}

	void disableEdit()
	{
		_valueEdit.disableEdit();
	}
};

class MapEditorHUD : public spk::Widget
{
private:
	class InterfaceContent : public spk::AbstractInterfaceWindow::Content
	{
	private:
		spk::TextLabel _layerTextlabel;
		Spinbox<int> _layerSpinBox;

		spk::Font::Size defaultFontSize()
		{
			return {16, 0};
		}

		void _onGeometryChange() override
		{
			spk::Font::Size fontSize = defaultFontSize();

			_layerTextlabel.setTextSize(fontSize);
			spk::Vector2UInt minimalTextLabelSize = 
				_layerTextlabel.computeExpectedTextSize(fontSize) + 
				_layerTextlabel.cornerSize() * 2;

			_layerTextlabel.setGeometry({0, 0}, minimalTextLabelSize);

			const unsigned int spinboxWidth = geometry().size.x - 5 - minimalTextLabelSize.x;
			spk::Vector2UInt spinboxSize = {spinboxWidth, minimalTextLabelSize.y};

			_layerSpinBox.setGeometry(
				{
					_layerTextlabel.geometry().anchor.x + 
					_layerTextlabel.geometry().size.x + 3,
					0
				},
				spinboxSize
			);
		}

	public:
		InterfaceContent(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent)
			: spk::AbstractInterfaceWindow::Content(p_name, p_parent),
			_layerTextlabel(L"Layer text label", this),
			_layerSpinBox(L"Layer spinbox", this)
		{
			_layerTextlabel.setText(L"Layer :");
			_layerTextlabel.setTextAlignment(
				spk::HorizontalAlignment::Right,
				spk::VerticalAlignment::Centered
			);
			_layerTextlabel.setCornerSize(2);
			_layerTextlabel.activate();

			_layerSpinBox.setValue(0);
			_layerSpinBox.setStep(1);
			_layerSpinBox.setCornerSize(2);
			_layerSpinBox.setLimits(0, 5);
			_layerSpinBox.disableEdit();
			_layerSpinBox.activate();
		}

		spk::Vector2UInt minimalSize()
		{
			spk::Vector2UInt labelSize =
				_layerTextlabel.computeExpectedTextSize(defaultFontSize()) + 
				_layerTextlabel.cornerSize() * 2;

			const unsigned int spinboxWidth = 100;
			spk::Vector2UInt spinboxSize = {spinboxWidth, labelSize.y};

			unsigned int totalWidth = labelSize.x + 70;

			unsigned int totalHeight = std::max(labelSize.y, spinboxSize.y);

			return {totalWidth, totalHeight};
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