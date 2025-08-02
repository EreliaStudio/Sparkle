#include <sparkle.hpp>
#include <spk_debug_macro.hpp>

enum class Event
{
	MoveObject
};

using EventDispatcher = spk::EventDispatcher<Event>;

class Component : public spk::Component
{
public:
	enum class ActionType
	{
		Action
	};

private:
	EventDispatcher::Instanciator _eventDispatcherInstanciator;

	std::unordered_map<ActionType, std::unique_ptr<spk::Action>> _actions;

	spk::SafePointer<spk::KeyboardAction> _testAction;

	spk::Camera _camera;
	spk::Entity _cameraHolder;

	spk::ContractProvider::Contract _onEditionCallback;

protected:
	template <typename Enum>
	spk::SafePointer<spk::Action> _setAction(Enum p_value, std::unique_ptr<spk::Action> &&p_action)
	{
		spk::SafePointer<spk::Action> result = p_action.get();
		_actions.emplace(p_value, std::move(p_action));
		return (result);
	}

public:
	Component(const std::wstring &p_name) :
		spk::Component(p_name),
		_cameraHolder(L"CameraHolder", nullptr)
	{
		_cameraHolder.transform().place({0, 0, 10});
		_cameraHolder.transform().lookAt({0, 0, 0});

		_testAction = _setAction(ActionType::Action, std::make_unique<spk::KeyboardAction>(spk::Keyboard::Z, spk::InputState::Down, 1000,
			[&](const spk::SafePointer<const spk::Keyboard>& p_keyboard) {
				spk::cout << "Treating the input event" << std::endl;
				EventDispatcher::instance()->emit(Event::MoveObject, spk::Vector2(0, 1));
			})).upCast<spk::KeyboardAction>();
	}

	void awake() override
	{
		owner()->addChild(&_cameraHolder);

		_onEditionCallback = owner()->transform().addOnEditionCallback([&](){
			spk::cout << "Executing the transform edition callback of [" << owner()->name() << "] -> New position [" << owner()->transform().position() << "]" << std::endl;
		});
	}

	void onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		for (auto &[key, action] : _actions)
		{
			if (action->isInitialized() == false)
			{
				action->initialize(p_event);
			}

			action->update();
		}
	}
};

class TestObject : public spk::Entity
{
private:
	spk::SafePointer<Component> _controller;
	EventDispatcher::Instanciator _eventDispatcherInstanciator;

public:
	TestObject(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent),
		_controller(addComponent<Component>(L"Component"))
	{
		EventDispatcher::instance()->subscribe(Event::MoveObject, [&](spk::Vector2 p_delta){
			spk::cout << "Receiving event move object" << std::endl;
			transform().move(spk::Vector3(p_delta, 0));
		}).relinquish();
	}
};

class TestWidget : public spk::Screen
{
private:
	spk::GameEngineWidget _gameEngineWidget;
	spk::GameEngine _engine;

	TestObject _tmpObject;

	void _onGeometryChange()
	{
		_gameEngineWidget.setGeometry({}, geometry().size);
	}

public:
	TestWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Screen(p_name, p_parent),
		_gameEngineWidget(p_name + L"/GameEngineWidget", this),
		_tmpObject(L"TestObject", nullptr)
	{
		_gameEngineWidget.setGameEngine(&_engine);
		_gameEngineWidget.activate();

		_tmpObject.transform().place({0, 0, 0});
		_tmpObject.activate();
		
		_engine.addEntity(&_tmpObject);
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 600}});

	TestWidget testWidget = TestWidget(L"TestWidget", win->widget());
	testWidget.setGeometry({0, 0}, win->geometry().size);
	testWidget.activate();

	return (app.run());
}