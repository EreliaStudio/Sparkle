#include "playground.hpp"

class MapEditorWidget : public spk::Widget
{
private:
	spk::SafePointer<MapEditorManager> _mapEditorManager = nullptr;

public:
	MapEditorWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{

	}

	void bind(spk::SafePointer<MapEditorManager> p_mapEditorManager)
	{
		_mapEditorManager = p_mapEditorManager;
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
	auto& mapEditorManagerComp = worldManager.addComponent<MapEditorManager>(L"Map editor component");
	mapEditorManagerComp.setOnClickLambda([&worldManagerComp](const spk::Vector2Int& p_tilePosition){
		spk::Vector2Int chunkPosition = Chunk::convertWorldToChunkPosition(p_tilePosition);
		worldManagerComp.chunkEntity(chunkPosition)->chunk()->setContent(p_tilePosition.positiveModulo(Chunk::Dimensions.xy()), 0, 1);
		worldManagerComp.invalidateChunk(chunkPosition);
		spk::cout << "Setting tile [" << p_tilePosition << "] to [1]" << std::endl;
	});

	spk::Entity player = spk::Entity(L"Player");
	player.transform().place(spk::Vector3(0, 0, 0));
	player.addComponent<PlayerController>(L"Player controler component");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, 20));
	camera.transform().lookAt(player.transform().position());

	CameraManager& cameraComp = camera.addComponent<CameraManager>(L"Main camera");

	mapEditorManagerComp.setCameraManager(&cameraComp);
	worldManagerComp.setCamera(&camera);

	engine.addEntity(&worldManager);
	engine.addEntity(&player);

	spk::InterfaceWindow mapEditorWindow = spk::InterfaceWindow(L"Editor window", win->widget());
	mapEditorWindow.setMenuHeight(26);
	mapEditorWindow.setLayer(10);
	mapEditorWindow.setGeometry({0, 0}, {250, 500});
	mapEditorWindow.activate();

	MapEditorWidget mapEditorWidget = MapEditorWidget(L"Editor window", mapEditorWindow.content());
	mapEditorWidget.bind(&mapEditorManagerComp);
	mapEditorWidget.setGeometry({0, 0}, mapEditorWindow.content()->geometry().size);
	mapEditorWidget.activate();

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setLayer(0);
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();
	
	return (app.run());
}