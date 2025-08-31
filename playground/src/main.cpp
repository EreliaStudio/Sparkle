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

// Simple 2D top-down controller using ZQSD to move an entity on the XY plane
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

	// Player entity with 2D top-down controller
	spk::Entity player(L"Player", nullptr);
	player.addComponent<TopDown2DController>(L"Player/TopDown2DController");

	// Camera holder as a child of the player so it follows automatically
	spk::Entity cameraHolder(L"Camera", &player);
	auto cameraComponent = cameraHolder.addComponent<spk::CameraComponent>(L"Camera/CameraComponent");
	cameraHolder.transform().place({0.0f, 0.0f, 20.0f});
	cameraHolder.transform().lookAt(player.transform().position());

	try
	{
		cameraComponent->camera().setOrthographic(-80.0f, 80.0f, -80.0f, 80.0f);
	} catch (const std::exception &e)
	{
		PROPAGATE_ERROR("Error while computing the camera as orthographic", e);
	}

	PlaygroundTileMap tileMap(L"TileMap", nullptr);
	tileMap.activate();
	engine.addEntity(&tileMap);

	spk::SpriteSheet spriteSheet("playground/resources/texture/tile_map.png", {5, 6});
	tileMap.setSpriteSheet(&spriteSheet);

	tileMap.addTileByID(1, PlaygroundTileMap::TileType({0, 0}, PlaygroundTileMap::TileType::Type::Monotile));
	tileMap.addTileByID(0, PlaygroundTileMap::TileType({1, 0}, PlaygroundTileMap::TileType::Type::Autotile));

	tileMap.setChunkRange({-5, -5}, {5, 5});

	player.activate();
	cameraHolder.activate();
	engine.addEntity(&player);

	return app.run();
}
