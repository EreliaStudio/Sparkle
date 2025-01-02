#include "playground.hpp"

int executeMain()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	TextureManager::instanciate();
	
	NodeMap nodeMap;

	nodeMap.addNode(0, {
	 	.animationStartPos = spk::Vector2(0, 0),
		.frameDuration = 1000,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.addNode(1, {
	 	.animationStartPos = spk::Vector2(1, 1),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.addNode(2, {
	 	.animationStartPos = spk::Vector2(1, 0),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.addNode(3, {
	 	.animationStartPos = spk::Vector2(0, 1),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.validate();

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
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}

int main()
{
	int result = executeMain();
	return (result);
}