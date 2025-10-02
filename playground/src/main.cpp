#include "playground.hpp"

#include <memory>

enum class Event
{
	OnWorldUnitResize,
	OnCameraGeometryEdition
};

using EventDispatcher = spk::Singleton<spk::EventDispatcher<Event>>;

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
	static constexpr int kPrimaryLayer = 0;
	spk::RandomGenerator<int> _interiorTileIdGenerator;

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

	spk::SafePointer<Activator> _activator;

public:
	explicit Tilemap(const std::wstring &p_name) :
		spk::TileMap<16, 16, 3, PlaygroundTileFlag>(p_name, nullptr),
		_activator(addComponent<Activator>(p_name + L"/Activator")),
		_interiorTileIdGenerator()
	{
		_interiorTileIdGenerator.configureRange(1, 3);
		_activator->activate();
	}

protected:
	void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
	{
		(void)p_chunkCoordinate;

		for (int y = 0; y < Chunk::size.y; ++y)
		{
			for (int x = 0; x < Chunk::size.x; ++x)
			{
				const bool isEdge = (x == 0) || (y == 0) || (x == Chunk::size.x - 1) || (y == Chunk::size.y - 1);
				if (isEdge == true)
				{
					p_chunkToFill.setContent(x, y, kPrimaryLayer, 0);
				}
				else
				{
					const short randomTileId = static_cast<short>(_interiorTileIdGenerator.sample());
					p_chunkToFill.setContent(x, y, kPrimaryLayer, randomTileId);
				}
			}
		}

		p_chunkToFill.unbake();
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

	Player player = Player(L"Player");
	player.transform().place(spk::Vector2(0, 0));
	player.transform().setLayer(5);
	Tilemap tilemap = Tilemap(L"Tilemap");
	tilemap.transform().setLayer(0);

	gameEngine.addEntity(&player);
	gameEngine.addEntity(&tilemap);

	player.activate();
	tilemap.activate();

	spk::SpriteSheet spriteSheet(L"playground/resources/texture/tile_map.png", spk::Vector2UInt(16, 6));
	tilemap.setSpriteSheet(&spriteSheet);

	using TileType = Tilemap::TileType;
	tilemap.addTileByID(0, TileType(spk::Vector2UInt(0, 0), TileType::Type::Autotile));
	tilemap.addTileByID(1, TileType(spk::Vector2UInt(4, 0), TileType::Type::Autotile));
	tilemap.addTileByID(2, TileType(spk::Vector2UInt(8, 0), TileType::Type::Autotile));
	tilemap.addTileByID(3, TileType(spk::Vector2UInt(12, 0), TileType::Type::Autotile));

	EventDispatcher::instance()->emit(Event::OnWorldUnitResize);

	return (app.run());
}
