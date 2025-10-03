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

	struct City
	{
		enum class Type
		{
			Gym,  // Main city
			City, // Normal town
			Town, // Small town
			POI	  // Point of interest
		};

		struct GenerationData
		{
			float areaRadius;	   // The distance to the "center" of the town on which i should place "type" as tile value
			float territoryRadius; // The distance to the "center" of the town on which i should place "type + 1" as tile value
			float influenceRadius; // The distance to the "center" of the town on which i should place "type + 2" as tile value
		};

		Type type;
		spk::Vector2 position;
		City *parent;
		std::vector<City *> children;
	};

	std::vector<std::unique_ptr<City>> _cities;
	std::vector<spk::Vector2Int> _cityCenters;

	const std::unordered_map<City::Type, City::GenerationData> _cityGenerationDatas = {
		{City::Type::Gym, {50, 100, 200}}, {City::Type::City, {30, 50, 100}}, {City::Type::Town, {15, 30, 50}}, {City::Type::POI, {5, 10, 20}}};

	void _setupMap(const spk::Vector2Int &p_size)
	{
		_cities.clear();
		_cityCenters.clear();

		std::mt19937 rng(std::random_device{}());
		const spk::Vector2 mapSize(static_cast<float>(p_size.x), static_cast<float>(p_size.y));

		auto baseTileId = [](City::Type p_type) -> int
		{
			switch (p_type)
			{
			case City::Type::Gym:
				return 0;
			case City::Type::City:
				return 3;
			case City::Type::Town:
				return 6;
			case City::Type::POI:
				return 9;
			}
			return 0;
		};

		auto clampToMap = [&](spk::Vector2 p_position, float p_radius)
		{
			p_position.x = std::clamp(p_position.x, p_radius, mapSize.x - p_radius);
			p_position.y = std::clamp(p_position.y, p_radius, mapSize.y - p_radius);
			return p_position;
		};

		auto isInsideMap = [&](const spk::Vector2 &p_position, float p_radius)
		{
			if ((p_position.x < p_radius) || (p_position.y < p_radius))
			{
				return false;
			}
			if ((p_position.x > mapSize.x - p_radius) || (p_position.y > mapSize.y - p_radius))
			{
				return false;
			}
			return true;
		};

		std::vector<City *> placedCities;

		auto isValidPosition = [&](const spk::Vector2 &p_candidate, const City::GenerationData &p_data, City::Type p_type, const City *p_parent)
		{
			for (City *existing : placedCities)
			{
				if (existing == p_parent)
				{
					continue;
				}

				const City::GenerationData &existingData = _cityGenerationDatas.at(existing->type);
				float requiredDistance = p_data.territoryRadius + existingData.territoryRadius;
				float dx = p_candidate.x - existing->position.x;
				float dy = p_candidate.y - existing->position.y;
				float distanceSquared = dx * dx + dy * dy;
				if (distanceSquared < requiredDistance * requiredDistance)
				{
					return false;
				}
			}
			return true;
		};

		auto applyCityTiles = [&](const City &p_city)
		{
			const City::GenerationData &data = _cityGenerationDatas.at(p_city.type);
			const int tileBase = baseTileId(p_city.type);

			auto paintCircle = [&](float p_radius, int p_offset)
			{
				if (p_radius <= 0.0f)
				{
					return;
				}

				const int minX = std::max(0, static_cast<int>(std::floor(p_city.position.x - p_radius)));
				const int maxX = std::min(p_size.x - 1, static_cast<int>(std::ceil(p_city.position.x + p_radius)));
				const int minY = std::max(0, static_cast<int>(std::floor(p_city.position.y - p_radius)));
				const int maxY = std::min(p_size.y - 1, static_cast<int>(std::ceil(p_city.position.y + p_radius)));

				for (int y = minY; y <= maxY; ++y)
				{
					for (int x = minX; x <= maxX; ++x)
					{
						float dx = static_cast<float>(x) - p_city.position.x;
						float dy = static_cast<float>(y) - p_city.position.y;
						float distance = std::sqrt(dx * dx + dy * dy);
						if (distance <= p_radius)
						{
							setContent(x, y, 0, tileBase + p_offset);
						}
					}
				}
			};

			//paintCircle(data.influenceRadius, 2);
			//paintCircle(data.territoryRadius, 1);
			paintCircle(data.areaRadius, 0);
		};

		auto makeCity = [&](City::Type p_type, const spk::Vector2 &p_position, City *p_parent)
		{
			auto city = std::make_unique<City>();
			city->type = p_type;
			city->position = p_position;
			city->parent = p_parent;

			City *cityPtr = city.get();
			if (p_parent != nullptr)
			{
				p_parent->children.push_back(cityPtr);
			}

			_cityCenters.push_back(spk::Vector2Int(static_cast<int>(std::round(p_position.x)), static_cast<int>(std::round(p_position.y))));

			applyCityTiles(*city);

			placedCities.push_back(cityPtr);
			_cities.push_back(std::move(city));

			return cityPtr;
		};

		auto randomRange = [&](float p_min, float p_max)
		{
			if (p_max < p_min)
			{
				std::swap(p_min, p_max);
			}
			std::uniform_real_distribution<float> dist(p_min, p_max);
			return dist(rng);
		};

		auto placeCity = [&](City::Type p_type, City *p_parent)
		{
			const City::GenerationData &data = _cityGenerationDatas.at(p_type);
			City *result = nullptr;

			const int maxAttempts = 8000;
			for (int attempt = 0; (attempt < maxAttempts) && (result == nullptr); ++attempt)
			{
				spk::Vector2 candidate;

				if (p_parent == nullptr)
				{
					float minX = data.influenceRadius;
					float maxX = mapSize.x - data.influenceRadius;
					float minY = data.influenceRadius;
					float maxY = mapSize.y - data.influenceRadius;
					candidate = spk::Vector2(randomRange(minX, maxX), randomRange(minY, maxY));
				}
				else
				{
					const City::GenerationData &parentData = _cityGenerationDatas.at(p_parent->type);
					float minRadius = parentData.territoryRadius + data.areaRadius;
					float maxRadius = parentData.influenceRadius - data.influenceRadius;

					if (maxRadius < minRadius)
					{
						maxRadius = minRadius + data.influenceRadius;
					}

					float radius = randomRange(minRadius, maxRadius);
					float angle = randomRange(0.0f, 2.0f * M_PI);

					candidate = spk::Vector2(p_parent->position.x + std::cos(angle) * radius, p_parent->position.y + std::sin(angle) * radius);
				}

				candidate = clampToMap(candidate, data.influenceRadius);

				if ((isInsideMap(candidate, data.influenceRadius) == false) || (isValidPosition(candidate, data, p_type, p_parent) == false))
				{
					continue;
				}

				result = makeCity(p_type, candidate, p_parent);
			}

			if (result == nullptr)
			{
				spk::Vector2 fallback = (p_parent == nullptr) ? spk::Vector2(mapSize.x / 2.0f, mapSize.y / 2.0f) : p_parent->position;
				fallback = clampToMap(fallback, data.influenceRadius);
				result = makeCity(p_type, fallback, p_parent);
			}

			return result;
		};

		std::vector<City *> gyms;
		gyms.reserve(8);

		for (int gymIndex = 0; gymIndex < 8; ++gymIndex)
		{
			gyms.push_back(placeCity(City::Type::Gym, nullptr));
		}

		std::vector<City *> cities;
		for (City *gym : gyms)
		{
			for (int i = 0; i < 2; ++i)
			{
				cities.push_back(placeCity(City::Type::City, gym));
			}
		}

		std::vector<City *> towns;
		for (City *city : cities)
		{
			for (int i = 0; i < 2; ++i)
			{
				towns.push_back(placeCity(City::Type::Town, city));
			}
		}

		auto gatherAnchors = [&](City *p_root)
		{
			std::vector<City *> anchors;
			auto gatherRecursive = [&](auto &&self, City *p_node) -> void
			{
				anchors.push_back(p_node);
				for (City *child : p_node->children)
				{
					self(self, child);
				}
			};

			gatherRecursive(gatherRecursive, p_root);
			return anchors;
		};

		for (City *gym : gyms)
		{
			std::vector<City *> anchors = gatherAnchors(gym);
			if (anchors.empty() == true)
			{
				anchors.push_back(gym);
			}

			for (int i = 0; i < 4; ++i)
			{
				std::uniform_int_distribution<size_t> indexDist(0, anchors.size() - 1);
				City *anchor = anchors[indexDist(rng)];
				placeCity(City::Type::POI, anchor);
			}
		}
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
		addTileByID(0, TileType(spk::Vector2UInt(0, 0), TileType::Type::Monotile));	 // Big city
		addTileByID(1, TileType(spk::Vector2UInt(1, 0), TileType::Type::Monotile));	 // Big city territoty
		addTileByID(2, TileType(spk::Vector2UInt(2, 0), TileType::Type::Monotile));	 // Big city influence
		addTileByID(3, TileType(spk::Vector2UInt(0, 1), TileType::Type::Monotile));	 // Town
		addTileByID(4, TileType(spk::Vector2UInt(1, 1), TileType::Type::Monotile));	 // Town territoty
		addTileByID(5, TileType(spk::Vector2UInt(2, 1), TileType::Type::Monotile));	 // Town influence
		addTileByID(6, TileType(spk::Vector2UInt(0, 2), TileType::Type::Monotile));	 // Small town
		addTileByID(7, TileType(spk::Vector2UInt(1, 2), TileType::Type::Monotile));	 // Small town territoty
		addTileByID(8, TileType(spk::Vector2UInt(2, 2), TileType::Type::Monotile));	 // Small town influence
		addTileByID(9, TileType(spk::Vector2UInt(0, 3), TileType::Type::Monotile));	 // Point Of Interest
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
