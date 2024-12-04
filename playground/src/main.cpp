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

public:
	explicit Tilemap(const std::wstring &p_name) :
		spk::TileMap<16, 16, 3, PlaygroundTileFlag>(p_name, nullptr),
		_assetAtlasInstanciator(),
		_activator(addComponent<Activator>(p_name + L"/Activator"))
	{
		setSpriteSheet(AssetAtlas::instance()->spriteSheet(L"ChunkTileset"));
		
		using TileType = Tilemap::TileType;
		addTileByID(0, TileType(spk::Vector2UInt(0, 0), TileType::Type::Monotile)); // Big city
		addTileByID(1, TileType(spk::Vector2UInt(1, 0), TileType::Type::Monotile)); // Town
		addTileByID(2, TileType(spk::Vector2UInt(2, 0), TileType::Type::Monotile)); // Small town

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
					p_chunkToFill.setContent(x, y, 0, 0);
				}
				else
				{
					p_chunkToFill.setContent(x, y, 0, 1);
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

	EventDispatcher::instance()->emit(Event::OnWorldUnitResize);

	return (app.run());
}
