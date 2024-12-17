#include "playground.hpp"

class TestComponent : public spk::Component
{
private:

public:
	TestComponent() :
		spk::Component(L"TestComponent")
	{

	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {840, 680}});

	spk::GameEngine engine;

	spk::Entity player(L"Player");
	player.addComponent<TestComponent>();
	
	spk::Entity camera(L"Camera", &player);

	spk::Entity cube(L"Cube");

	engine.addEntity(&player);
	engine.addEntity(&cube);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}