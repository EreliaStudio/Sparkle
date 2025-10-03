#include "playground.hpp"

#include <memory>

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
	CameraComponent(const std::wstring& p_name) :
		spk::CameraComponent2D(p_name)
	{
		setPixelsPerUnit({3, 3});

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
		Activator(const std::wstring& p_name) :
			spk::Component(p_name)
		{
			_onWorldUnitResizeEventContract = _eventDispatcherInstanciator->subscribe(Event::OnWorldUnitResize, [&](){
				_updateChunkVisibility();
			});
			_onCameraGeometryEditionEventContract = _eventDispatcherInstanciator->subscribe(Event::OnCameraGeometryEdition, [&](){
				_updateChunkVisibility();
			});
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

	std::vector<spk::Vector2Int> citiesPositions;

	struct City
	{
		enum class Type
		{
			Gym, // Main city
			City, // Normal town
			Town, // Small town
			POI // Point of interest
		};
		
		struct GenerationData
		{
			float areaRadius; // The distance to the "center" of the town on which i should place "type" as tile value
			float territoryRadius; // The distance to the "center" of the town on which i should place "type + 1" as tile value
			float influenceRadius; // The distance to the "center" of the town on which i should place "type + 2" as tile value
		};

		spk::Vector2Int position;
		City* parent;
		std::vector<City*> children;
	};

	const std::unordered_map<City::Type, City::GenerationData> _cityGenerationDatas = {
		{City::Type::Gym, {50, 100, 200}},
		{City::Type::City, {30, 50, 100}},
		{City::Type::Town, {15, 30, 50}},
		{City::Type::POI, {5, 10, 20}}
	};

	void _setupMap(const spk::Vector2Int& p_size)
	{
		//Generate 8 gym city, than generate 2 City for each gym, than 2 town for each city, and than 4 POI for each gym.
		// The idea is that i want to poisson-disk the cities on the whole map, and set the tiles to visualy see the town, its "territory", where no other stuff could be placed, its "influence", where i can place another object.
		// When you create a City linked to a Gym, i need you to add the influence of the city to the "influance" of the Gym, allow you to place the next town, city or POI using this city too.
	
		// Note that i need to have the stuff distributed in a poisson-disk algorythmn, to have them evenly distributed
	}

public:
	explicit Tilemap(const std::wstring &p_name, const spk::Vector2Int& p_size) :
		spk::TileMap<16, 16, 3, PlaygroundTileFlag>(p_name, nullptr),
		_assetAtlasInstanciator(),
		_activator(addComponent<Activator>(p_name + L"/Activator"))
	{
		_setupMap(p_size);

		setSpriteSheet(AssetAtlas::instance()->spriteSheet(L"ChunkTileset"));
		
		using TileType = Tilemap::TileType;
		addTileByID(0, TileType(spk::Vector2UInt(0, 0), TileType::Type::Monotile)); // Big city
		addTileByID(1, TileType(spk::Vector2UInt(1, 0), TileType::Type::Monotile)); // Big city territoty
		addTileByID(2, TileType(spk::Vector2UInt(2, 0), TileType::Type::Monotile)); // Big city influence
		addTileByID(3, TileType(spk::Vector2UInt(0, 1), TileType::Type::Monotile)); // Town
		addTileByID(4, TileType(spk::Vector2UInt(1, 1), TileType::Type::Monotile)); // Town territoty
		addTileByID(5, TileType(spk::Vector2UInt(2, 1), TileType::Type::Monotile)); // Town influence
		addTileByID(6, TileType(spk::Vector2UInt(0, 2), TileType::Type::Monotile)); // Small town
		addTileByID(7, TileType(spk::Vector2UInt(1, 2), TileType::Type::Monotile)); // Small town territoty
		addTileByID(8, TileType(spk::Vector2UInt(2, 2), TileType::Type::Monotile)); // Small town influence
		addTileByID(9, TileType(spk::Vector2UInt(0, 3), TileType::Type::Monotile)); // Point Of Interest
		addTileByID(10, TileType(spk::Vector2UInt(1, 3), TileType::Type::Monotile)); // Point Of Interest territoty
		addTileByID(11, TileType(spk::Vector2UInt(2, 3), TileType::Type::Monotile)); // Point Of Interest influence

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

	spk::Vector2Int mapSize = {2000, 600}; // In tile
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
