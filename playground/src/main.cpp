#include "playground.hpp"

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	taag::AssetAtlas::instanciate();

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({{0, 0}, window->geometry().size});
	engineWidget.setGameEngine(&engine);
	engineWidget.activate();

	engine.rootObject()->addComponent<taag::Shape::Renderer>(L"Shape renderer");

	taag::Player player(L"Player", nullptr);
	player.camera()->setOrthographic({20, 20});
	player.setType(taag::Shape::Type::Triangle);
	engine.addEntity(&player);
	player.transform().place({4, 4, 2.5f});
	player.activate();

	taag::TileMap tileMap(L"TileMap", nullptr);
	engine.addEntity(&tileMap);
	tileMap.activate();
	tileMap.setSpriteSheet(taag::AssetAtlas::instance()->spriteSheet(L"DefaultTileMapTexture"));
	tileMap.addTileByID(0, taag::TileMap::TileType({0, 0}, taag::TileMap::TileType::Type::Autotile, taag::TileFlag::Obstacle));
	tileMap.addTileByID(1, taag::TileMap::TileType({4, 0}, taag::TileMap::TileType::Type::Autotile, taag::TileFlag::TerritoryBlue));
	tileMap.addTileByID(2, taag::TileMap::TileType({8, 0}, taag::TileMap::TileType::Type::Autotile, taag::TileFlag::TerritoryGreen));
	tileMap.addTileByID(3, taag::TileMap::TileType({12, 0}, taag::TileMap::TileType::Type::Autotile, taag::TileFlag::TerritoryRed));
	tileMap.generate({15, 15});

	return app.run();
}