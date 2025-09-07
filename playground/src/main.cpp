#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <list>
#include <memory>
#include <numbers>
#include <sparkle.hpp>
#include <unordered_map>
#include <vector>

class Shape : public spk::Entity
{
public:
	enum class Type
	{
		Triangle, 
		Square,
		Pentagon,
		Hexagon,
		Octogon,
		Circle
	};

private:
	using ShapeMesh = spk::TMesh<spk::Vector2>;

	struct Info
	{
		spk::Matrix4x4 model;
		spk::Color color;
	};

	using InfoContainer = std::list<Info>;
	using InfoIterator = InfoContainer::iterator;

	static ShapeMesh makeMesh(const Shape::Type& p_type)
	{
		//Create a mesh based on the number of point corresponding to the shape passed as parameter
	}

	struct Renderer : public spk::Component
	{
	private:
		class Painter
		{
		private:
			static inline spk::Lumina::ShaderObjectFactory::Instanciator _instanciator;

			static spk::Lumina::Shader _createShader()
			{
				const char *vertexShaderSrc = R"(#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform CameraUBO
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} cameraUBO;

layout(std140, binding = 1) uniform TransformUBO
{
	mat4 modelMatrix; // base model matrix (e.g., for the whole batch)
} transformUBO;

struct Info
{
	mat4 model;
	vec4 color;
};

layout(std430, binding = 2) buffer InfoSSBO
{
	Info object[];
};

void main()
{
	mat4 model = transformUBO.modelMatrix * object[gl_InstanceIndex].model;

	vec4 worldPos = model * vec4(inPosition, 0.0, 1.0);
	gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;

	fragColor = object[gl_InstanceIndex].color;
})";

				const char *fragmentShaderSrc = R"(const char *fragmentShaderSrc = R"(#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor;
})";

				spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);

				return (shader);
			}
			static inline spk::Lumina::Shader _shader = _createShader();

			spk::Lumina::Shader::Object _object;
			spk::OpenGL::ShaderStorageBufferObject& _infoSSBO;

			spk::SafePointer<const InfoContainer> _infoContainer;

			void prepare(const ShapeMesh& p_mesh)
			{

			}

		public:
			Painter(const ShapeMesh& p_mesh) :
				_object(_shader.createObject()),
				_infoSSBO(_object.SSBO(L"InfoSSBO"))
			{
				//Push the mesh as data buffer
			}
			
			void render()
			{
				// Render the object using the info SSBO size as number of instances for the object render call
			}

			void setShapeList(const spk::SafePointer<const InfoContainer> p_infoContainer)
			{
				_infoContainer = p_infoContainer;
			}
			
			void validate()
			{
				//Validate the SSBO, updating the SSBO content with the new content of the linked list
			}
		};

		static inline std::unordered_map<Shape::Type, Painter> painters = {
			{Shape::Type::Triangle, Painter(makeMesh(Shape::Type::Triangle))},
			{Shape::Type::Square, Painter(makeMesh(Shape::Type::Square))},
			{Shape::Type::Pentagon, Painter(makeMesh(Shape::Type::Pentagon))},
			{Shape::Type::Hexagon, Painter(makeMesh(Shape::Type::Hexagon))},
			{Shape::Type::Octogon, Painter(makeMesh(Shape::Type::Octogon))},
			{Shape::Type::Circle, Painter(makeMesh(Shape::Type::Circle))}
		};

		struct ContainerData
		{
			bool needUpdate = false;
			InfoContainer container;
		};

		static inline std::unordered_map<Shape::Type, ContainerData> containers = {
			{Shape::Type::Triangle, ContainerData()},
			{Shape::Type::Square, ContainerData()},
			{Shape::Type::Pentagon, ContainerData()},
			{Shape::Type::Hexagon, ContainerData()},
			{Shape::Type::Octogon, ContainerData()},
			{Shape::Type::Circle, ContainerData()}
		};

	public:
		Renderer(const std::wstring& p_name) :
			spk::Component(p_name)
		{

		}

		void onPaintEvent(spk::PaintEvent& p_event) override
		{
			for (auto& painter : painters)
			{
				painter.second.render();
			}
		}

		static InfoIterator subscribe(const Shape::Type& p_type)
		{
			//Add the iterator to the adequat list
		}

		static void remove(const Shape::Type& p_type, const InfoIterator& p_iterator)
		{
			//Remove the iterator from the adequat list
		}

		static void validate(const Shape::Type& p_type)
		{
			containers[p_type].needUpdate = true;
		}
	};

	class Subscriber : public spk::Component
	{
	private:
		std::optional<Shape::Type> _type;
		InfoIterator _iterator;

		void bind()
		{
			_iterator = Renderer::subscribe(_type.value());
		}

		void unbind()
		{
			Renderer::remove(_type.value(), _iterator);
		}

	public:
		Subscriber(const std::wstring& p_name) : 
			spk::Component(p_name)
		{
			_type.reset();
		}

		void setShapeType(const Shape::Type& p_type)
		{
			if (_type.has_value() == true)
			{
				unbind();
			}

			_type = p_type;

			bind();
		}

		void awake() override
		{
			bind();
		}

		void sleep() override
		{
			unbind();
		}
	};

private:
	spk::SafePointer<Subscriber> _subscriber;
	Type _type;

public:
	Shape(const std::wstring& p_name, spk::SafePointer<spk::Entity> p_parent) : 
		spk::Entity(p_name, p_parent),
		_subscriber(addComponent<Subscriber>(p_name + L"/Subscriber"))
	{

	}

	void setType(const Type& p_type)
	{
		_type = p_type;
	}
};

enum class TileFlag
{
	None,
	Obstacle,
	TerritoryBlue,
	TerritoryGreen,
	TerritoryRed
};

class PlaygroundTileMap : public spk::TileMap<16, 16, 4, TileFlag>
{
private:
	void _onChunkGeneration(const spk::Vector2Int &p_chunkCoordinate, Chunk &p_chunkToFill) override
	{
		for (int i = 0; i < Chunk::size.x; i++)
		{
			for (int j = 0; j < Chunk::size.y; j++)
			{
				if (i == 0 || j == 0)
				{
					p_chunkToFill.setContent(i, j, 0, 0);
				}
				else
				{
					p_chunkToFill.setContent(i, j, 0, (rand() % 3) + 1);
				}
			}
		}

		auto collisionManager = p_chunkToFill.addComponent<Chunk::Collider>(p_chunkToFill.name() + L"/Collider");
		collisionManager->setFlag(TileFlag::Obstacle);
	}

public:
	using spk::TileMap<16, 16, 4, TileFlag>::TileMap;

	PlaygroundTileMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::TileMap<16, 16, 4, TileFlag>(p_name, p_parent)
	{
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

			auto toWorld = [&](const spk::Vector2 &p_ndc) -> spk::Vector3
			{
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
	TileMapChunkStreamer(
		const std::wstring &p_name, spk::SafePointer<PlaygroundTileMap> p_tileMap, spk::SafePointer<spk::CameraComponent> p_cameraComponent) :
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

class DebugOverlayManager : public spk::Widget
{
private:
	spk::Profiler::Instanciator _profilerInstanciator;
	spk::DebugOverlay _debugOverlay;

	void _updateOutlineFromLayout()
	{
		const uint32_t labelHeight = _debugOverlay.labelHeight();
		const uint32_t outlineSize = std::max<uint32_t>(1u, labelHeight / 6u);
		_debugOverlay.setFontOutlineSize(outlineSize);
	}

	void _onGeometryChange() override
	{
		spk::Vector2UInt s = geometry().size;
		s.y = std::min(s.y, _debugOverlay.computeMaxHeightPixels());
		_debugOverlay.setGeometry({{0, 0}, s});
		_updateOutlineFromLayout();
	}

	void _onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		size_t fps = 0;
		double fpsMs = 0.0;
		double fpsMinMs = 0.0;
		double fpsMaxMs = 0.0;
		size_t fpsMin = 0;
		size_t fpsMax = 0;

		size_t ups = 0;
		double upsMs = 0.0;
		double upsMinMs = 0.0;
		double upsMaxMs = 0.0;
		size_t upsMin = 0;
		size_t upsMax = 0;

		if (p_event.window != nullptr)
		{
			spk::SafePointer<spk::Window> wnd = p_event.window;
			if (wnd != nullptr)
			{
				fps = wnd->FPS();
				fpsMs = wnd->realFPSDuration();
				fpsMinMs = wnd->minFPSDuration();
				fpsMaxMs = wnd->maxFPSDuration();
				fpsMin = wnd->minFPS();
				fpsMax = wnd->maxFPS();

				ups = wnd->UPS();
				upsMs = wnd->realUPSDuration();
				upsMinMs = wnd->minUPSDuration();
				upsMaxMs = wnd->maxUPSDuration();
				upsMin = wnd->minUPS();
				upsMax = wnd->maxUPS();
			}
		}

		_debugOverlay.setText(0, 0, L"FPS Counter : " + std::to_wstring(fps));
		_debugOverlay.setText(1, 0, L"FPS Duration : " + std::to_wstring(fpsMs) + L" ms");
		_debugOverlay.setText(2, 0, L"Min duration : " + std::to_wstring(fpsMinMs) + L" ms");
		_debugOverlay.setText(3, 0, L"Max duration : " + std::to_wstring(fpsMaxMs) + L" ms");
		_debugOverlay.setText(4, 0, L"Min FPS : " + std::to_wstring(fpsMin));
		_debugOverlay.setText(5, 0, L"Max FPS : " + std::to_wstring(fpsMax));
		_debugOverlay.setText(6, 0, L"");
		_debugOverlay.setText(7, 0, L"UPS Counter : " + std::to_wstring(ups));
		_debugOverlay.setText(8, 0, L"UPS Duration : " + std::to_wstring(upsMs) + L" ms");
		_debugOverlay.setText(9, 0, L"Min duration : " + std::to_wstring(upsMinMs) + L" ms");
		_debugOverlay.setText(10, 0, L"Max duration : " + std::to_wstring(upsMaxMs) + L" ms");
		_debugOverlay.setText(11, 0, L"Min UPS : " + std::to_wstring(upsMin));
		_debugOverlay.setText(12, 0, L"Max UPS : " + std::to_wstring(upsMax));

		p_event.requestPaint();
	}

public:
	DebugOverlayManager(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_debugOverlay(p_name + L"/Overlay", this)
	{
		_debugOverlay.setMaxGlyphSize(30);
		_debugOverlay.setFontOutlineSize(4);
		_debugOverlay.setFontOutlineSharpness(1);
		_debugOverlay.setFontColor(spk::Color::white, spk::Color::black);

		_debugOverlay.configureRows(20, 1);
		_debugOverlay.activate();
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);

	DebugOverlayManager debugOverlay(L"DebugOverlay", window->widget());
	debugOverlay.setGeometry({0, 0}, window->geometry().size);
	debugOverlay.setLayer(100);
	debugOverlay.activate();

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({{0, 0}, window->geometry().size});
	engineWidget.setGameEngine(&engine);
	engineWidget.activate();

	spk::Entity player(L"Player", nullptr);
	player.transform().place({4, 4, 2.5f});
	player.addComponent<TopDown2DController>(L"Player/TopDown2DController");

	spk::SpriteSheet playerSpriteSheet("playground/resources/texture/player_sprite_sheet.png", {4, 4});
	spk::Mesh2D playerMesh = spk::Primitive2D::makeSquare({1.0f, 1.0f}, playerSpriteSheet.sprite({0, 0}));
	spk::CollisionMesh2D playerCollisionMesh = spk::CollisionMesh2D::fromMesh(&playerMesh);
	auto playerRenderer = player.addComponent<spk::Mesh2DRenderer>(L"Player/Mesh2DRenderer");
	playerRenderer->setTexture(&playerSpriteSheet);
	playerRenderer->setMesh(&playerMesh);
	auto playerCollider = player.addComponent<spk::Collider2D>(L"Player/Collider2D");
	playerCollider->setCollisionMesh(&playerCollisionMesh);

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
	spk::SpriteSheet tilemapSpriteSheet("playground/resources/texture/tile_map.png", {16, 6});
	tileMap.activate();
	engine.addEntity(&tileMap);
	tileMap.setSpriteSheet(&tilemapSpriteSheet);

	tileMap.addTileByID(0, PlaygroundTileMap::TileType({0, 0}, PlaygroundTileMap::TileType::Type::Autotile, TileFlag::Obstacle)); // Wall
	tileMap.addTileByID(
		1, PlaygroundTileMap::TileType({4, 0}, PlaygroundTileMap::TileType::Type::Autotile, TileFlag::TerritoryBlue)); // Blue territory
	tileMap.addTileByID(
		2, PlaygroundTileMap::TileType({8, 0}, PlaygroundTileMap::TileType::Type::Autotile, TileFlag::TerritoryGreen)); // Green territory
	tileMap.addTileByID(
		3, PlaygroundTileMap::TileType({12, 0}, PlaygroundTileMap::TileType::Type::Autotile, TileFlag::TerritoryRed)); // Red territory

	player.addComponent<TileMapChunkStreamer>(L"Player/TileMapChunkStreamer", &tileMap, cameraComponent);

	spk::SafePointer<spk::Entity> playerPtr(&player);
	for (const auto &collider : spk::Collider2D::getColliders())
	{
		if (collider.get() == playerCollider.get())
		{
			continue;
		}
		collider->onCollisionEnter(
			[playerPtr](spk::SafePointer<spk::Entity> p_entity)
			{
				if (p_entity.get() == playerPtr.get())
				{
					std::wcout << L"Player entered chunk collision mesh" << std::endl;
				}
			});
	}

	player.activate();
	cameraHolder.activate();
	engine.addEntity(&player);

	spk::Entity shapesRoot(L"Shapes", nullptr);
	shapesRoot.addComponent<playground::Shape::Renderer>(L"Shapes/ShapeRenderer");
	engine.addEntity(&shapesRoot);
	shapesRoot.activate();

	std::vector<std::unique_ptr<playground::Shape>> shapes;
	auto createShapes = [&](playground::Shape::Type p_type,
							const std::array<spk::Color, 3> &p_colors,
							const std::array<spk::Vector3, 3> &p_positions,
							const std::wstring &p_base)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			auto shape = std::make_unique<playground::Shape>(p_base + std::to_wstring(i), p_type, p_colors[i], &shapesRoot);
			shape->transform().place(p_positions[i]);
			shape->activate();
			shapes.push_back(std::move(shape));
		}
	};

	std::array<spk::Color, 3> colors = {spk::Color::red, spk::Color::green, spk::Color::blue};
	createShapes(
		playground::Shape::Type::Triangle,
		colors,
		{spk::Vector3(-9.0f, -9.0f, 0.0f), spk::Vector3(0.0f, -9.0f, 0.0f), spk::Vector3(9.0f, -9.0f, 0.0f)},
		L"Triangle");
	createShapes(
		playground::Shape::Type::Square,
		colors,
		{spk::Vector3(-9.0f, -5.0f, 0.0f), spk::Vector3(0.0f, -5.0f, 0.0f), spk::Vector3(9.0f, -5.0f, 0.0f)},
		L"Square");
	createShapes(
		playground::Shape::Type::Pentagon,
		colors,
		{spk::Vector3(-9.0f, -1.0f, 0.0f), spk::Vector3(0.0f, -1.0f, 0.0f), spk::Vector3(9.0f, -1.0f, 0.0f)},
		L"Pentagon");
	createShapes(
		playground::Shape::Type::Hexagon,
		colors,
		{spk::Vector3(-9.0f, 3.0f, 0.0f), spk::Vector3(0.0f, 3.0f, 0.0f), spk::Vector3(9.0f, 3.0f, 0.0f)},
		L"Hexagon");
	createShapes(
		playground::Shape::Type::Octagon,
		colors,
		{spk::Vector3(-9.0f, 7.0f, 0.0f), spk::Vector3(0.0f, 7.0f, 0.0f), spk::Vector3(9.0f, 7.0f, 0.0f)},
		L"Octagon");
	createShapes(
		playground::Shape::Type::Circle,
		colors,
		{spk::Vector3(-9.0f, 9.0f, 0.0f), spk::Vector3(0.0f, 9.0f, 0.0f), spk::Vector3(9.0f, 9.0f, 0.0f)},
		L"Circle");

	return app.run();
}
