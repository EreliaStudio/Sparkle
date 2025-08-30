#include <sparkle.hpp>

enum class TileFlag
{
	None
};

class PlaygroundTileMap : public spk::TileMap<16, 16, 4, TileFlag>
{
private:
	void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
	{
		for (int y = 0; y < Chunk::size.y; ++y)
		{
			for (int x = 0; x < Chunk::size.x; ++x)
			{
				TileType::ID id = 0;
				if (x == 0 || y == 0 || x == Chunk::size.x - 1 || y == Chunk::size.y - 1)
				{
					id = 1;
				}
				p_chunkToFill.setContent(x, y, 0, id);
				for (int layer = 1; layer < static_cast<int>(LayerCount); ++layer)
				{
					p_chunkToFill.setContent(x, y, layer, -1);
				}
			}
		}
	}

public:
	using spk::TileMap<16, 16, 4, TileFlag>::TileMap;
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({{0, 0}, window->geometry().size});
	engineWidget.setGameEngine(&engine);
	engineWidget.activate();

	spk::Entity cameraEntity(L"Camera", nullptr);
	auto cameraComponent = cameraEntity.addComponent<spk::CameraComponent>(L"Camera/CameraComponent");
	cameraEntity.transform().place({0.0f, 0.0f, 20.0f});
	cameraEntity.transform().lookAt({0.0f, 0.0f, 0.0f});
	cameraEntity.activate();
	engine.addEntity(&cameraEntity);
	cameraComponent->camera().setOrthographic(-80.0f, 80.0f, -80.0f, 80.0f);

	PlaygroundTileMap tileMap(L"TileMap", nullptr);
	tileMap.activate();
	engine.addEntity(&tileMap);

	spk::SpriteSheet spriteSheet("playground/resources/texture/tile_map.png", {5, 6});
	tileMap.setSpriteSheet(&spriteSheet);

	tileMap.addTileByID(0, PlaygroundTileMap::TileType({0, 0}, PlaygroundTileMap::TileType::Type::Monotile));
	tileMap.addTileByID(1, PlaygroundTileMap::TileType({1, 0}, PlaygroundTileMap::TileType::Type::Autotile));

	tileMap.setChunkRange({-5, -5}, {5, 5});

	return app.run();
}
