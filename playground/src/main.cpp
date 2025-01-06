#include "playground.hpp"

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

	DEBUG_LINE();
	spk::PushButton editorPushButton = spk::PushButton(L"Editor push button", win->widget());
	editorPushButton.setLayer(100);
	DEBUG_LINE();
	editorPushButton.setCornerSize(8);
	DEBUG_LINE();
	editorPushButton.setGeometry({0, 100}, {100, 100});
	DEBUG_LINE();
	editorPushButton.setPressedSpriteSheet(TextureManager::instance()->spriteSheet(L"pushButtonNineSlice"));
	DEBUG_LINE();
	editorPushButton.setReleasedSpriteSheet(TextureManager::instance()->spriteSheet(L"pushButtonNineSlice"));
	DEBUG_LINE();
	editorPushButton.activate();
	DEBUG_LINE();

	return (app.run());
}