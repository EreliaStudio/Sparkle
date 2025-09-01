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
		// Hardcoded pattern: "crosses on cross" using tile ID 0 (autotile)
		// This intentionally uses a simple list of setContent calls for easy editing.
		// Coordinates are within a 9x9 block starting at (0,0).
		// Row 0
		p_chunkToFill.setContent(2, 0, 0, 0);
		p_chunkToFill.setContent(4, 0, 0, 0);
		p_chunkToFill.setContent(6, 0, 0, 0);
		// Row 1
		p_chunkToFill.setContent(3, 1, 0, 0);
		p_chunkToFill.setContent(4, 1, 0, 0);
		p_chunkToFill.setContent(5, 1, 0, 0);
		// Row 2
		p_chunkToFill.setContent(0, 2, 0, 0);
		p_chunkToFill.setContent(4, 2, 0, 0);
		p_chunkToFill.setContent(8, 2, 0, 0);
		// Row 3
		p_chunkToFill.setContent(1, 3, 0, 0);
		p_chunkToFill.setContent(3, 3, 0, 0);
		p_chunkToFill.setContent(4, 3, 0, 0);
		p_chunkToFill.setContent(5, 3, 0, 0);
		p_chunkToFill.setContent(7, 3, 0, 0);
		// Row 4
		p_chunkToFill.setContent(0, 4, 0, 0);
		p_chunkToFill.setContent(1, 4, 0, 0);
		p_chunkToFill.setContent(2, 4, 0, 0);
		p_chunkToFill.setContent(3, 4, 0, 0);
		p_chunkToFill.setContent(4, 4, 0, 0);
		p_chunkToFill.setContent(5, 4, 0, 0);
		p_chunkToFill.setContent(6, 4, 0, 0);
		p_chunkToFill.setContent(7, 4, 0, 0);
		p_chunkToFill.setContent(8, 4, 0, 0);
		// Row 5
		p_chunkToFill.setContent(1, 5, 0, 0);
		p_chunkToFill.setContent(3, 5, 0, 0);
		p_chunkToFill.setContent(4, 5, 0, 0);
		p_chunkToFill.setContent(5, 5, 0, 0);
		p_chunkToFill.setContent(7, 5, 0, 0);
		// Row 6
		p_chunkToFill.setContent(0, 6, 0, 0);
		p_chunkToFill.setContent(4, 6, 0, 0);
		p_chunkToFill.setContent(8, 6, 0, 0);
		// Row 7
		p_chunkToFill.setContent(3, 7, 0, 0);
		p_chunkToFill.setContent(4, 7, 0, 0);
		p_chunkToFill.setContent(5, 7, 0, 0);
		// Row 8
		p_chunkToFill.setContent(2, 8, 0, 0);
		p_chunkToFill.setContent(4, 8, 0, 0);
		p_chunkToFill.setContent(6, 8, 0, 0);
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
	player.transform().place({4, 4, 2.5f});
	player.addComponent<TopDown2DController>(L"Player/TopDown2DController");

	// Camera holder as a child of the player so it follows automatically
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

	spk::SpriteSheet spriteSheet("playground/resources/texture/tile_map.png", {5, 6});
	tileMap.setSpriteSheet(&spriteSheet);

	tileMap.addTileByID(1, PlaygroundTileMap::TileType({0, 0}, PlaygroundTileMap::TileType::Type::Monotile));
	tileMap.addTileByID(0, PlaygroundTileMap::TileType({1, 0}, PlaygroundTileMap::TileType::Type::Autotile));

	tileMap.setChunkRange({-0, -0}, {0, 0});

	player.activate();
	cameraHolder.activate();
	engine.addEntity(&player);

	return app.run();
}
