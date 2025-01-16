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

	spk::InterfaceWindow window = spk::InterfaceWindow(L"Editor inventory", win->widget());
	window.setMenuHeight(24);
	window.setGeometry({100, 100}, win->geometry().size / 2);
	window.activate();

	window.maximize();

	spk::TextLabel innerTextLabel = spk::TextLabel(L"Inner text label", window.content());
	innerTextLabel.setGeometry({-100, 100}, {300, 50});
	innerTextLabel.setText(L"My text realy long to be sure to see it");
	innerTextLabel.activate();

	spk::ContractProvider::Contract _windowSuppressionContract = window.subscribeTo(spk::InterfaceWindow::Close, [&](){spk::cout << "Deletion of window" << std::endl;});

	return (app.run());
}