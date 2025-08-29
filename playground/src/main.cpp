#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sparkle.hpp>
#include <unordered_map>
#include <vector>

struct FullBlock : public spk::Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite front;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite top;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;

public:
	explicit FullBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/full_block.obj");

		_applySprite(_objMesh.shapes()[0], _configuration.front);
		_applySprite(_objMesh.shapes()[1], _configuration.back);
		_applySprite(_objMesh.shapes()[2], _configuration.left);
		_applySprite(_objMesh.shapes()[3], _configuration.right);
		_applySprite(_objMesh.shapes()[4], _configuration.top);
		_applySprite(_objMesh.shapes()[5], _configuration.bottom);
	}
};

struct SlopeBlock : public spk::Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite triangleLeft;
		spk::SpriteSheet::Sprite triangleRight;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite ramp;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;

public:
	explicit SlopeBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/slope_block.obj");

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.back);
		_applySprite(_objMesh.shapes()[2], _configuration.triangleLeft);
		_applySprite(_objMesh.shapes()[3], _configuration.triangleRight);
		_applySprite(_objMesh.shapes()[4], _configuration.ramp);
	}
};

struct HalfBlock : public spk::Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite front;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite top;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;

public:
	explicit HalfBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/half_block.obj");

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.top);
		_applySprite(_objMesh.shapes()[2], _configuration.left);
		_applySprite(_objMesh.shapes()[3], _configuration.left);
		_applySprite(_objMesh.shapes()[4], _configuration.left);
		_applySprite(_objMesh.shapes()[5], _configuration.left);
	}
};

struct StairBlock : public spk::Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite bottom;
		spk::SpriteSheet::Sprite staircaseTop;
		spk::SpriteSheet::Sprite staircaseFront;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite back;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;

public:
	explicit StairBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/stair_block.obj");

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.staircaseTop);
		_applySprite(_objMesh.shapes()[2], _configuration.staircaseTop);
		_applySprite(_objMesh.shapes()[3], _configuration.staircaseFront);
		_applySprite(_objMesh.shapes()[4], _configuration.staircaseFront);
		_applySprite(_objMesh.shapes()[5], _configuration.back);
		_applySprite(_objMesh.shapes()[6], _configuration.left);
		_applySprite(_objMesh.shapes()[7], _configuration.left);
		_applySprite(_objMesh.shapes()[8], _configuration.right);
		_applySprite(_objMesh.shapes()[9], _configuration.right);
	}
};

class TmpBlockMap : public spk::BlockMap<16, 16, 16>
{
private:
	void _onChunkGeneration(const spk::Vector3Int &p_chunkCoordinate, Chunk &p_chunkToFill)
	{
		for (size_t i = 0; i < Chunk::size.x; i++)
		{
			for (size_t j = 0; j < Chunk::size.z; j++)
			{
				p_chunkToFill.setContent(i, 0, j, 0);
				if (i == 0 && j != 0)
				{
					p_chunkToFill.setContent(
						i,
						1,
						j,
						2,
						spk::Block::Orientation{spk::Block::HorizontalOrientation::ZNegative, spk::Block::VerticalOrientation::YPositive});
				}
				if (j == 0 && i != 0)
				{
					p_chunkToFill.setContent(
						i,
						1,
						j,
						3,
						spk::Block::Orientation{spk::Block::HorizontalOrientation::ZPositive, spk::Block::VerticalOrientation::YNegative});
				}
				if (i == 1 && j != 1)
				{
					p_chunkToFill.setContent(
						i,
						1,
						j,
						1,
						spk::Block::Orientation{spk::Block::HorizontalOrientation::XPositive, spk::Block::VerticalOrientation::YPositive});
				}
				if (j == 1 && i != 1)
				{
					p_chunkToFill.setContent(
						i,
						1,
						j,
						1,
						spk::Block::Orientation{spk::Block::HorizontalOrientation::ZPositive, spk::Block::VerticalOrientation::YPositive});
				}
				if (i == 0 && j == 0)
				{
					p_chunkToFill.setContent(i, 1, j, 0);
					p_chunkToFill.setContent(i, 2, j, 0);
					p_chunkToFill.setContent(i, 3, j, 0);
					p_chunkToFill.setContent(i, 4, j, 0);
				}
			}
		}
	}

public:
	TmpBlockMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::BlockMap<16, 16, 16>(p_name, p_parent)
	{
	}
};

class Player : public spk::Entity
{
private:
	spk::SafePointer<spk::CameraComponent> _cameraComponent = nullptr;
	spk::SafePointer<spk::FreeViewController> _freeViewController = nullptr;

public:
	Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent)
	{
		_cameraComponent = addComponent<spk::CameraComponent>(L"Player/CameraComponent");
		_freeViewController = addComponent<spk::FreeViewController>(L"Player/FreeViewController");
	}

	spk::SafePointer<spk::CameraComponent> cameraComponent()
	{
		return (_cameraComponent);
	}

	spk::SafePointer<spk::FreeViewController> freeViewController()
	{
		return (_freeViewController);
	}
};

template <size_t ChunkSizeX, size_t ChunkSizeY, size_t ChunkSizeZ>
class CollisionRenderToggler : public spk::Component
{
private:
	spk::SafePointer<spk::BlockMap<ChunkSizeX, ChunkSizeY, ChunkSizeZ>> _blockMap;
	bool _useCollisionRenderer = false;

public:
	CollisionRenderToggler(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void setBlockMap(spk::SafePointer<spk::BlockMap<ChunkSizeX, ChunkSizeY, ChunkSizeZ>> p_blockMap)
	{
		_blockMap = p_blockMap;
	}

	void onKeyboardEvent(spk::KeyboardEvent &p_event) override
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press && p_event.key == spk::Keyboard::F1)
		{
			_useCollisionRenderer = (_useCollisionRenderer == false);
			if (_blockMap != nullptr)
			{
				_blockMap->useCollisionRenderer(_useCollisionRenderer);
			}
		}
	}
};

class RayCastPrinter : public spk::Component
{
private:
	spk::SafePointer<spk::CameraComponent> _cameraComponent;
	spk::Entity _cubeEntity;
	spk::ColorMesh _cubeMesh;
	bool _isCasting = false;

	void _updateCubePosition(const spk::Vector2Int &p_mousePosition)
	{
		spk::Vector3 cameraDirection = _cameraComponent->camera().convertScreenToCamera(spk::Viewport::convertScreenToOpenGL(p_mousePosition));

		const auto &camMtx = _cameraComponent->owner()->transform().model();
		spk::Vector3 worldDirection = (camMtx * spk::Vector4(cameraDirection, 0.0f)).xyz().normalize();

		auto hit = spk::RayCast::launch(owner(), worldDirection, 1000.0f);
		if (hit.entity != nullptr)
		{
			_cubeEntity.transform().place(hit.position);
		}
	}

public:
	RayCastPrinter(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	spk::SafePointer<spk::Entity> cubeEntity()
	{
		return (&_cubeEntity);
	}

	void start() override
	{
		_cameraComponent = owner()->getComponent<spk::CameraComponent>();

		owner()->engine()->addEntity(&_cubeEntity);

		_cubeEntity.transform().place({3, 3, 3});
		_cubeEntity.setName(L"RayCastCube");
		_cubeEntity.activate();

		auto renderer = _cubeEntity.addComponent<spk::ColorMeshRenderer>(L"RayCastCube/ColorMeshRenderer");
		renderer->activate();

		float s = 0.1f;
		spk::Color color = spk::Color::red;

		using CV = spk::ColorVertex;
		CV v1{{-s, -s, -s}, color};
		CV v2{{s, -s, -s}, color};
		CV v3{{s, s, -s}, color};
		CV v4{{-s, s, -s}, color};
		CV v5{{-s, -s, s}, color};
		CV v6{{s, -s, s}, color};
		CV v7{{s, s, s}, color};
		CV v8{{-s, s, s}, color};

		_cubeMesh.clear();

		_cubeMesh.addShape(v1, v4, v3, v2); // Back
		_cubeMesh.addShape(v5, v6, v7, v8); // Front
		_cubeMesh.addShape(v1, v5, v8, v4); // Left
		_cubeMesh.addShape(v2, v3, v7, v6); // Right
		_cubeMesh.addShape(v4, v8, v7, v3); // Top
		_cubeMesh.addShape(v1, v2, v6, v5); // Bottom

		renderer->setMesh(&_cubeMesh);
	}

	void onMouseEvent(spk::MouseEvent &p_event) override
	{
		if (_cameraComponent == nullptr)
		{
			return;
		}

		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Press:
			if (p_event.button == spk::Mouse::Button::Right)
			{
				_isCasting = true;
				_updateCubePosition(p_event.mouse->position());
			}
			break;
		case spk::MouseEvent::Type::Motion:
			if (_isCasting == true)
			{
				_updateCubePosition(p_event.position);
			}
			break;
		case spk::MouseEvent::Type::Release:
			if (p_event.button == spk::Mouse::Button::Right)
			{
				_isCasting = false;
			}
			break;
		default:
			break;
		}
	}
};

class DebugOverlayManager : public spk::Widget
{
private:
	spk::Profiler::Instanciator _profilerInstanciator;
	spk::DebugOverlay<3, 20> _debugOverlay;

	void _onGeometryChange() override
	{
		_debugOverlay.setGeometry({{0, 0}, geometry().size});
	}

	void _onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		_debugOverlay.setText(0, 0, L"FPS : " + std::to_wstring(spk::Profiler::instance()->counter(L"FPS")->value()));
		_debugOverlay.setText(0, 1, L"UPS : " + std::to_wstring(spk::Profiler::instance()->counter(L"UPS")->value()));
		p_event.requestPaint();
	}

public:
	DebugOverlayManager(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_debugOverlay(p_name + L"/Overlay", this)
	{
		_debugOverlay.activate();
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);
	window->requestMousePlacement({400, 300});

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(&engine);
	engineWidget.setLayer(0);
	engineWidget.activate();

	DebugOverlayManager debugOverlay(L"DebugOverlay", window->widget());
	debugOverlay.setGeometry({0, 0}, window->geometry().size);
	debugOverlay.setLayer(100);
	debugOverlay.activate();

	Player player = Player(L"Player", nullptr);
	player.activate();
	engine.addEntity(&player);

	auto rayCastPrinter = player.addComponent<RayCastPrinter>(L"Player/RayCastPrinter");
	rayCastPrinter->activate();

	player.cameraComponent()->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	player.transform().place({5.0f, 5.0f, 5.0f});
	player.transform().lookAt({3.0f, 3.0f, 3.0f});

	TmpBlockMap blockMap = TmpBlockMap(L"BlockMap", nullptr);
	blockMap.setTexture(spk::Block::texture());
	blockMap.activate();
	engine.addEntity(&blockMap);

	auto fullBlockSprite = spk::Block::spriteSheet().sprite({0, 0});
	FullBlock::Configuration fullConfiguration;
	fullConfiguration.front = fullBlockSprite;
	fullConfiguration.back = fullBlockSprite;
	fullConfiguration.left = fullBlockSprite;
	fullConfiguration.right = fullBlockSprite;
	fullConfiguration.top = fullBlockSprite;
	fullConfiguration.bottom = fullBlockSprite;
	blockMap.addBlockByID(0, std::make_unique<FullBlock>(fullConfiguration));

	SlopeBlock::Configuration slopeConfiguration;
	slopeConfiguration.triangleLeft = spk::Block::spriteSheet().sprite({1, 0});
	slopeConfiguration.triangleRight = spk::Block::spriteSheet().sprite({1, 0});
	slopeConfiguration.back = spk::Block::spriteSheet().sprite({2, 0});
	slopeConfiguration.ramp = spk::Block::spriteSheet().sprite({2, 0});
	slopeConfiguration.bottom = spk::Block::spriteSheet().sprite({3, 0});
	blockMap.addBlockByID(1, std::make_unique<SlopeBlock>(slopeConfiguration));

	StairBlock::Configuration stairConfiguration;
	stairConfiguration.staircaseTop = spk::Block::spriteSheet().sprite({7, 0});
	stairConfiguration.staircaseFront = spk::Block::spriteSheet().sprite({7, 0});
	stairConfiguration.left = spk::Block::spriteSheet().sprite({6, 0});
	stairConfiguration.right = spk::Block::spriteSheet().sprite({6, 0});
	stairConfiguration.back = spk::Block::spriteSheet().sprite({8, 0});
	stairConfiguration.bottom = spk::Block::spriteSheet().sprite({8, 0});
	blockMap.addBlockByID(2, std::make_unique<StairBlock>(stairConfiguration));

	HalfBlock::Configuration halfConfiguration;
	halfConfiguration.front = spk::Block::spriteSheet().sprite({4, 0});
	halfConfiguration.back = spk::Block::spriteSheet().sprite({4, 0});
	halfConfiguration.left = spk::Block::spriteSheet().sprite({4, 0});
	halfConfiguration.right = spk::Block::spriteSheet().sprite({4, 0});
	halfConfiguration.top = spk::Block::spriteSheet().sprite({5, 0});
	halfConfiguration.bottom = spk::Block::spriteSheet().sprite({5, 0});
	blockMap.addBlockByID(3, std::make_unique<HalfBlock>(halfConfiguration));

	blockMap.setChunkRange({-0, 0, -0}, {0, 0, 0});
	// blockMap.setChunkRange({-3, 0, -3}, {3, 0, 3});

	auto collisionRenderToggler = player.addComponent<CollisionRenderToggler<16, 16, 16>>(L"Player/CollisionRenderToggler");
	collisionRenderToggler->setBlockMap(&blockMap);

	return app.run();
}
