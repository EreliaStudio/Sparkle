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

	spk::PushButton editorPushButton = spk::PushButton(L"Editor push button", win->widget());
	editorPushButton.subscribe([&](){spk::cout << "Button clicked" << std::endl;}).relinquish();
	editorPushButton.setLayer(100);
	editorPushButton.setCornerSize(4);
	editorPushButton.setText(L"ABCDEFG");
	editorPushButton.setTextSize({20, 4});
	editorPushButton.setTextColor(spk::Color::white, spk::Color::black);
	editorPushButton.setGeometry({300, 300}, {50, 50});
	editorPushButton.setPressedOffset(2);
	editorPushButton.setSpriteSheet(TextureManager::instance()->spriteSheet(L"pushButtonNineSlice"));
	editorPushButton.setFont(TextureManager::instance()->font(L"defaultFont"));
	editorPushButton.activate();

	return (app.run());
}