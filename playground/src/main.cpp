#include "playground.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

enum class Event
{
	OnWorldUnitResize,
	OnCameraGeometryEdition
};

using EventDispatcher = spk::Singleton<spk::EventDispatcher<Event>>;

class AssetAtlas : public spk::AssetAtlas, public spk::Singleton<AssetAtlas>
{
	friend class spk::Singleton<AssetAtlas>;

private:
	AssetAtlas()
	{
		load(L"playground/resources/assets.json");
	}
};

class CameraComponent : public spk::CameraComponent2D
{
private:
	EventDispatcher::Instanciator _eventDispatcherInstanciator;
	static inline spk::SafePointer<CameraComponent> _activeCameraComponent = nullptr;

public:
	CameraComponent(const std::wstring &p_name) :
		spk::CameraComponent2D(p_name)
	{
		setPixelsPerUnit({1, 1});

		if (activeCameraComponent() == nullptr)
		{
			setAsActive();
		}
	}

	void setAsActive()
	{
		_activeCameraComponent = this;
	}

	void setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit)
	{
		CameraComponent2D::setPixelsPerUnit(p_pixelsPerUnit);
		_eventDispatcherInstanciator->emit(Event::OnWorldUnitResize);
	}

	void onGeometryChange(const spk::Geometry2D &p_geometry) override
	{
		CameraComponent2D::onGeometryChange(p_geometry);
		_eventDispatcherInstanciator->emit(Event::OnCameraGeometryEdition);
	}

	static spk::SafePointer<CameraComponent> activeCameraComponent()
	{
		return (_activeCameraComponent);
	}
};

enum class PlaygroundTileFlag
{
	None = 0
};

class Player final : public spk::Entity2D
{
private:
	EventDispatcher::Instanciator _eventDispatcherInstanciator;
	spk::SafePointer<CameraComponent> _cameraComponent;

public:
	explicit Player(const std::wstring &p_name) :
		spk::Entity2D(p_name)
	{
		_cameraComponent = addComponent<CameraComponent>(p_name + L"/CameraComponent");
	}

	spk::SafePointer<spk::CameraComponent2D> cameraComponent() const
	{
		return (_cameraComponent);
	}
};

class Tilemap final : public spk::TileMap<16, 16, 3, PlaygroundTileFlag>
{
private:
	class Activator : public spk::Component
	{
	private:
		EventDispatcher::Instanciator _eventDispatcherInstanciator;
		spk::EventDispatcher<Event>::Contract<> _onWorldUnitResizeEventContract;
		spk::EventDispatcher<Event>::Contract<> _onCameraGeometryEditionEventContract;
		spk::SafePointer<spk::TileMap<16, 16, 3, PlaygroundTileFlag>> _ownerAsTilemap;

		void _updateChunkVisibility()
		{
			spk::Vector2 downLeftCorner = CameraComponent::activeCameraComponent()->ndcToWorld(spk::Vector2(-1, -1));
			spk::Vector2 topRightCorner = CameraComponent::activeCameraComponent()->ndcToWorld(spk::Vector2(1, 1));

			spk::Vector2Int downLeftChunk = Tilemap::worldToChunkCoordinates(downLeftCorner);
			spk::Vector2Int topRightChunk = Tilemap::worldToChunkCoordinates(topRightCorner);

			_ownerAsTilemap->activateChunks(downLeftChunk, topRightChunk);
		}

	public:
		Activator(const std::wstring &p_name) :
			spk::Component(p_name)
		{
			_onWorldUnitResizeEventContract = _eventDispatcherInstanciator->subscribe(Event::OnWorldUnitResize, [&]() { _updateChunkVisibility(); });
			_onCameraGeometryEditionEventContract =
				_eventDispatcherInstanciator->subscribe(Event::OnCameraGeometryEdition, [&]() { _updateChunkVisibility(); });
		}

		void start() override
		{
			_ownerAsTilemap = owner().upCast<spk::TileMap<16, 16, 3, PlaygroundTileFlag>>();
		}

		void onGeometryChange(const spk::Geometry2D &p_geometry) override
		{
			//_updateChunkVisibility();
		}
	};

	AssetAtlas::Instanciator _assetAtlasInstanciator;
	spk::SafePointer<Activator> _activator;

	void _setupMap(const spk::Vector2Int &p_size)
	{
		
	}

public:
	explicit Tilemap(const std::wstring &p_name, const spk::Vector2Int &p_size) :
		spk::TileMap<16, 16, 3, PlaygroundTileFlag>(p_name, nullptr),
		_assetAtlasInstanciator(),
		_activator(addComponent<Activator>(p_name + L"/Activator"))
	{
		_setupMap(p_size);

		setSpriteSheet(AssetAtlas::instance()->spriteSheet(L"ChunkTileset"));

		using TileType = Tilemap::TileType;
		addTileByID(0, TileType(spk::Vector2UInt(0, 0), TileType::Type::Monotile));	 // Fire biome - Gym
		addTileByID(1, TileType(spk::Vector2UInt(1, 0), TileType::Type::Monotile));	 // Fire biome - City
		addTileByID(2, TileType(spk::Vector2UInt(2, 0), TileType::Type::Monotile));	 // Fire biome - Territory
		addTileByID(3, TileType(spk::Vector2UInt(3, 0), TileType::Type::Monotile));	 // Air biome - Gym
		addTileByID(4, TileType(spk::Vector2UInt(4, 0), TileType::Type::Monotile));	 // Air biome - City
		addTileByID(5, TileType(spk::Vector2UInt(5, 0), TileType::Type::Monotile));	 // Air biome - Territory
		
		addTileByID(6, TileType(spk::Vector2UInt(0, 1), TileType::Type::Monotile));	 // Water biome - Gym
		addTileByID(7, TileType(spk::Vector2UInt(1, 1), TileType::Type::Monotile));	 // Water biome - City
		addTileByID(8, TileType(spk::Vector2UInt(2, 1), TileType::Type::Monotile));	 // Water biome - Territory
		addTileByID(9, TileType(spk::Vector2UInt(3, 1), TileType::Type::Monotile));	 // Stone biome - Gym
		addTileByID(10, TileType(spk::Vector2UInt(4, 1), TileType::Type::Monotile)); // Stone biome - City
		addTileByID(11, TileType(spk::Vector2UInt(5, 1), TileType::Type::Monotile)); // Stone biome - Territory
		
		addTileByID(12, TileType(spk::Vector2UInt(0, 2), TileType::Type::Monotile)); // Plant biome - Gym
		addTileByID(13, TileType(spk::Vector2UInt(1, 2), TileType::Type::Monotile)); // Plant biome - City
		addTileByID(14, TileType(spk::Vector2UInt(2, 2), TileType::Type::Monotile)); // Plant biome - Territory
		addTileByID(15, TileType(spk::Vector2UInt(3, 2), TileType::Type::Monotile)); // Evil biome - Gym
		addTileByID(16, TileType(spk::Vector2UInt(4, 2), TileType::Type::Monotile)); // Evil biome - City
		addTileByID(17, TileType(spk::Vector2UInt(5, 2), TileType::Type::Monotile)); // Evil biome - Territory
		
		addTileByID(18, TileType(spk::Vector2UInt(0, 3), TileType::Type::Monotile)); // Fight biome - Gym
		addTileByID(19, TileType(spk::Vector2UInt(1, 3), TileType::Type::Monotile)); // Fight biome - City
		addTileByID(20, TileType(spk::Vector2UInt(2, 3), TileType::Type::Monotile)); // Fight biome - Territory
		addTileByID(21, TileType(spk::Vector2UInt(3, 3), TileType::Type::Monotile)); // Electricity biome - Gym
		addTileByID(22, TileType(spk::Vector2UInt(4, 3), TileType::Type::Monotile)); // Electricity biome - City
		addTileByID(23, TileType(spk::Vector2UInt(5, 3), TileType::Type::Monotile)); // Electricity biome - Territory
		
		addTileByID(24, TileType(spk::Vector2UInt(0, 4), TileType::Type::Monotile)); // Steel biome - Gym
		addTileByID(25, TileType(spk::Vector2UInt(1, 4), TileType::Type::Monotile)); // Steel biome - City
		addTileByID(26, TileType(spk::Vector2UInt(2, 4), TileType::Type::Monotile)); // Steel biome - Territory
		addTileByID(27, TileType(spk::Vector2UInt(3, 4), TileType::Type::Monotile)); // Ghost biome - Gym
		addTileByID(28, TileType(spk::Vector2UInt(4, 4), TileType::Type::Monotile)); // Ghost biome - City
		addTileByID(29, TileType(spk::Vector2UInt(5, 4), TileType::Type::Monotile)); // Ghost biome - Territory
		
		addTileByID(30, TileType(spk::Vector2UInt(0, 5), TileType::Type::Monotile)); // Fairy biome - Gym
		addTileByID(31, TileType(spk::Vector2UInt(1, 5), TileType::Type::Monotile)); // Fairy biome - City
		addTileByID(32, TileType(spk::Vector2UInt(2, 5), TileType::Type::Monotile)); // Fairy biome - Territory
		addTileByID(33, TileType(spk::Vector2UInt(3, 5), TileType::Type::Monotile)); // Ice biome - Gym
		addTileByID(34, TileType(spk::Vector2UInt(4, 5), TileType::Type::Monotile)); // Ice biome - City
		addTileByID(35, TileType(spk::Vector2UInt(5, 5), TileType::Type::Monotile)); // Ice biome - Territory

		_activator->activate();
	}

protected:
	void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
	{
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"TacticalActionArenaGame", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	spk::GameEngine gameEngine;

	spk::GameEngineWidget engineWidget = spk::GameEngineWidget(L"GameEngineWidget", window->widget());
	engineWidget.setGameEngine(&gameEngine);
	engineWidget.setGeometry(window->geometry());
	engineWidget.activate();

	spk::Vector2Int mapSize = {2000, 1200}; // In tile
	Player player = Player(L"Player");
	player.transform().place(mapSize / 2);
	player.transform().setLayer(5);
	Tilemap tilemap = Tilemap(L"Tilemap", mapSize);
	tilemap.transform().setLayer(0);

	gameEngine.addEntity(&player);
	gameEngine.addEntity(&tilemap);

	player.activate();
	tilemap.activate();

	EventDispatcher::instance()->emit(Event::OnWorldUnitResize);

	return (app.run());
}
