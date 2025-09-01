#include <sparkle.hpp>

enum class TileFlag
{
	None,
	Obstacle
};

class PlaygroundTileMap : public spk::TileMap<16, 16, 4, TileFlag>
{
private:
	spk::Perlin _perlin;
	float _noiseScale = 0.12f;

	short _tileIdFromNoise(float p_value) const
	{
		if (p_value < 0.15f)
		{
			return 0; // Deep water
		}
		if (p_value < 0.30f)
		{
			return 1; // Water
		}
		if (p_value < 0.35f)
		{
			return 2; // Beach
		}
		if (p_value < 0.60f)
		{
			return 3; // Plain
		}
		if (p_value < 0.75f)
		{
			return 4; // Forest
		}
		if (p_value < 0.90f)
		{
			return 5; // Montain
		}
		return 6; // Peak
	}

	void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
	{
		const spk::Vector2Int chunkOrigin = p_chunkCoordinate * Chunk::size;
		for (int y = 0; y < Chunk::size.y; ++y)
		{
			for (int x = 0; x < Chunk::size.x; ++x)
			{
				const spk::Vector2Int world = chunkOrigin + spk::Vector2Int(x, y);

				float n = _perlin.sample2D(static_cast<float>(world.x) * _noiseScale, static_cast<float>(world.y) * _noiseScale, 0.0f, 1.0f);
				short id = _tileIdFromNoise(n);
				p_chunkToFill.setContent(x, y, 0, id);
			}
		}
	}

public:
	using spk::TileMap<16, 16, 4, TileFlag>::TileMap;

	PlaygroundTileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::TileMap<16, 16, 4, TileFlag>(p_name, p_parent)
	{
		_perlin.setOctaves(6);
		_perlin.setPersistence(0.5f);
		_perlin.setLacunarity(2.3f);
		_perlin.setInterpolation(spk::Perlin::Interpolation::SmoothStep);

		_perlin.setSeed(1337u);
	}
};

class CollisionViewToggle : public spk::Component
{
private:
	spk::SafePointer<PlaygroundTileMap> _tileMap;
	bool _collisionMode = false;
	std::unique_ptr<spk::KeyboardAction> _toggleAction;

public:
	CollisionViewToggle(const std::wstring &p_name, spk::SafePointer<PlaygroundTileMap> p_tileMap) :
		spk::Component(p_name),
		_tileMap(p_tileMap)
	{
		if ((_tileMap != nullptr) == true)
		{
			_tileMap->setCollisionFlags(spk::Flags<TileFlag>(TileFlag::None));
		}

		_toggleAction = std::make_unique<spk::KeyboardAction>(
			spk::Keyboard::F1,
			spk::InputState::Down,
			-1,
			[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard)
			{
				_collisionMode = (_collisionMode == false);
				if (_tileMap != nullptr)
				{
					_tileMap->setRenderMode(_collisionMode);
					p_keyboard->window()->requestPaint();
				}
			});
	}

	void onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		if ((p_event.keyboard == nullptr) == true)
		{
			return;
		}
		if ((_toggleAction->isInitialized() == false) == true)
		{
			_toggleAction->initialize(p_event);
		}
		_toggleAction->update();
	}
};

class TileMapChunkStreamer : public spk::Component
{
private:
	spk::SafePointer<PlaygroundTileMap> _tileMap;
	spk::SafePointer<spk::CameraComponent> _cameraComponent;
	spk::ContractProvider::Contract _transformContract;
	spk::Vector2Int _lastChunk = spk::Vector2Int(0, 0);
	bool _hasLastChunk = false;

	spk::Vector2Int _computeChunk(const spk::Vector3 &p_worldPos) const
	{
		const spk::Vector2Int &size = PlaygroundTileMap::Chunk::size;
		int cx = static_cast<int>(std::floor(p_worldPos.x / static_cast<float>(size.x)));
		int cy = static_cast<int>(std::floor(p_worldPos.y / static_cast<float>(size.y)));
		return spk::Vector2Int(cx, cy);
	}

	void _refreshRange()
	{
		if (_tileMap == nullptr)
		{
			return;
		}

		spk::SafePointer<spk::Entity> ownerEntity = owner();
		if (ownerEntity == nullptr)
		{
			return;
		}

		spk::Vector2Int start;
		spk::Vector2Int end;

		if (_cameraComponent != nullptr)
		{
			const spk::Camera &cam = _cameraComponent->camera();
			const spk::Matrix4x4 &camModel = _cameraComponent->owner()->transform().model();

			auto toWorld = [&](const spk::Vector2 &p_ndc) -> spk::Vector3 {
				spk::Vector3 inCam = cam.convertScreenToCamera(p_ndc);
				return (camModel * spk::Vector4(inCam, 1.0f)).xyz();
			};

			spk::Vector3 wBL = toWorld(spk::Vector2(-1.0f, -1.0f));
			spk::Vector3 wBR = toWorld(spk::Vector2(1.0f, -1.0f));
			spk::Vector3 wTL = toWorld(spk::Vector2(-1.0f, 1.0f));
			spk::Vector3 wTR = toWorld(spk::Vector2(1.0f, 1.0f));

			float minX = std::min(std::min(wBL.x, wBR.x), std::min(wTL.x, wTR.x));
			float maxX = std::max(std::max(wBL.x, wBR.x), std::max(wTL.x, wTR.x));
			float minY = std::min(std::min(wBL.y, wBR.y), std::min(wTL.y, wTR.y));
			float maxY = std::max(std::max(wBL.y, wBR.y), std::max(wTL.y, wTR.y));

			const spk::Vector2Int &cs = PlaygroundTileMap::Chunk::size;
			start.x = static_cast<int>(std::floor(minX / static_cast<float>(cs.x))) - 1;
			start.y = static_cast<int>(std::floor(minY / static_cast<float>(cs.y))) - 1;
			end.x = static_cast<int>(std::floor(maxX / static_cast<float>(cs.x))) + 1;
			end.y = static_cast<int>(std::floor(maxY / static_cast<float>(cs.y))) + 1;
		}

		spk::Vector2Int curr = _computeChunk(ownerEntity->transform().position());
		if ((_hasLastChunk == true) && (curr == _lastChunk))
		{
			return;
		}

		_lastChunk = curr;
		_hasLastChunk = true;
		_tileMap->setChunkRange(start, end);
	}

public:
	TileMapChunkStreamer(const std::wstring &p_name, spk::SafePointer<PlaygroundTileMap> p_tileMap, spk::SafePointer<spk::CameraComponent> p_cameraComponent) :
		spk::Component(p_name),
		_tileMap(p_tileMap),
		_cameraComponent(p_cameraComponent)
	{
	}

	void awake() override
	{
		if (owner() != nullptr)
		{
			_transformContract = owner()->transform().addOnEditionCallback([&]() { _refreshRange(); });
		}

		_refreshRange();
	}

	void sleep() override
	{
		if (_transformContract.isValid())
		{
			_transformContract.resign();
		}
	}
};

class TopDown2DController : public spk::Component
{
public:
	struct Configuration
	{
		static inline const std::wstring ForwardActionName = L"Forward";
		static inline const std::wstring LeftActionName = L"Left";
		static inline const std::wstring BackwardActionName = L"Backward";
		static inline const std::wstring RightActionName = L"Right";

		float moveSpeed = 5.0f;
		std::unordered_map<std::wstring, spk::Keyboard::Key> keymap = {
			{ForwardActionName, spk::Keyboard::Z},
			{LeftActionName, spk::Keyboard::Q},
			{BackwardActionName, spk::Keyboard::S},
			{RightActionName, spk::Keyboard::D}};
	};

private:
	Configuration _config;
	std::vector<std::unique_ptr<spk::Action>> _actions;
	spk::Vector3 _motionRequested = {0, 0, 0};

	void _applyConfiguration()
	{
		dynamic_cast<spk::KeyboardAction *>(_actions[0].get())->setDeviceValue(_config.keymap[L"Forward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[1].get())->setDeviceValue(_config.keymap[L"Left"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[2].get())->setDeviceValue(_config.keymap[L"Backward"], spk::InputState::Down);
		dynamic_cast<spk::KeyboardAction *>(_actions[3].get())->setDeviceValue(_config.keymap[L"Right"], spk::InputState::Down);
	}

public:
	TopDown2DController(const std::wstring &p_name) :
		spk::Component(p_name)
	{
		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Forward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += spk::Vector3(0, 1, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Left"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= spk::Vector3(1, 0, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Backward"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested -= spk::Vector3(0, 1, 0); }));

		_actions.push_back(
			std::make_unique<spk::KeyboardAction>(
				_config.keymap[L"Right"],
				spk::InputState::Down,
				10,
				[&](const spk::SafePointer<const spk::Keyboard> &p_keyboard) { _motionRequested += spk::Vector3(1, 0, 0); }));
	}

	void setConfiguration(const Configuration &p_configuration)
	{
		_config = p_configuration;
		_applyConfiguration();
	}

	const Configuration &configuration() const
	{
		return (_config);
	}

	void onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		if ((p_event.keyboard == nullptr) == true)
		{
			return;
		}

		_motionRequested = spk::Vector3(0, 0, 0);

		for (auto &action : _actions)
		{
			if ((action->isInitialized() == false) == true)
			{
				action->initialize(p_event);
			}

			action->update();
		}

		bool isMotionRequested = (_motionRequested != spk::Vector3());
		if (isMotionRequested == true)
		{
			spk::Vector3 delta = _motionRequested.normalize() * (float)p_event.deltaTime.seconds * _config.moveSpeed;
			owner()->transform().move(delta);
			p_event.requestPaint();
		}
	}
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

	spk::Entity player(L"Player", nullptr);
	player.transform().place({4, 4, 2.5f});
	player.addComponent<TopDown2DController>(L"Player/TopDown2DController");

	spk::Entity cameraHolder(L"Camera", &player);
	auto cameraComponent = cameraHolder.addComponent<spk::CameraComponent>(L"Camera/CameraComponent");
	cameraHolder.transform().place({0.0f, 0.0f, 20.0f});
	cameraHolder.transform().lookAt(player.transform().position());

	try
	{
		cameraComponent->setOrthographic(64.0f, 64.0f);
	} catch (const std::exception &e)
	{
		PROPAGATE_ERROR("Error while computing the camera as orthographic", e);
	}

	PlaygroundTileMap tileMap(L"TileMap", nullptr);
	tileMap.activate();
	engine.addEntity(&tileMap);

	spk::SpriteSheet spriteSheet("playground/resources/texture/tile_map.png", {28, 6});
	tileMap.setSpriteSheet(&spriteSheet);

	tileMap.addTileByID(0, PlaygroundTileMap::TileType({0, 0}, PlaygroundTileMap::TileType::Type::Autotile));  // Deep water
	tileMap.addTileByID(1, PlaygroundTileMap::TileType({4, 0}, PlaygroundTileMap::TileType::Type::Autotile));  // Water
	tileMap.addTileByID(2, PlaygroundTileMap::TileType({8, 0}, PlaygroundTileMap::TileType::Type::Autotile));  // Beach
	tileMap.addTileByID(3, PlaygroundTileMap::TileType({12, 0}, PlaygroundTileMap::TileType::Type::Autotile)); // Plain
	tileMap.addTileByID(4, PlaygroundTileMap::TileType({16, 0}, PlaygroundTileMap::TileType::Type::Autotile)); // Forest
	tileMap.addTileByID(5, PlaygroundTileMap::TileType({20, 0}, PlaygroundTileMap::TileType::Type::Autotile)); // Montain
	tileMap.addTileByID(6, PlaygroundTileMap::TileType({24, 0}, PlaygroundTileMap::TileType::Type::Autotile)); // Peak

	player.addComponent<TileMapChunkStreamer>(L"Player/TileMapChunkStreamer", &tileMap, cameraComponent);

	player.addComponent<CollisionViewToggle>(L"Player/CollisionViewToggle", &tileMap);

	player.activate();
	cameraHolder.activate();
	engine.addEntity(&player);

	return app.run();
}
