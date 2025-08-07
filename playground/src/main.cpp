#include <array>
#include <memory>
#include <sparkle.hpp>
#include <unordered_map>
#include <vector>

#include "structure/engine/spk_voxel_mesh_renderer.hpp"

class Block
{
protected:
	std::vector<spk::SpriteSheet::Sprite> _sprites;

	static void _addQuad(spk::VoxelMesh &p_mesh,
						 const spk::SpriteSheet::Sprite &p_sprite,
						 const spk::Vector3 &p_a,
						 const spk::Vector3 &p_b,
						 const spk::Vector3 &p_c,
						 const spk::Vector3 &p_d)
	{
		spk::Vector2 anchor = p_sprite.anchor;
		spk::Vector2 size = p_sprite.size;
		p_mesh.addShape(spk::VoxelVertex{p_a, anchor},
						spk::VoxelVertex{p_b, anchor + spk::Vector2(size.x, 0)},
						spk::VoxelVertex{p_c, anchor + size},
						spk::VoxelVertex{p_d, anchor + spk::Vector2(0, size.y)});
	}

public:
	virtual ~Block() = default;

	virtual bool isVoxel() const
	{
		return (false);
	}

	virtual void insert(spk::VoxelMesh &p_mesh, const spk::Vector3 &p_base, const std::array<bool, 6> &p_visible) const = 0;
};

class VoxelBlock : public Block
{
public:
	VoxelBlock(const spk::SpriteSheet::Sprite &p_sprite)
	{
		_sprites.assign(6, p_sprite);
	}

	VoxelBlock(const spk::SpriteSheet::Sprite &p_top, const spk::SpriteSheet::Sprite &p_bottom, const spk::SpriteSheet::Sprite &p_side)
	{
		_sprites = {p_top, p_bottom, p_side, p_side, p_side, p_side};
	}

	VoxelBlock(const std::vector<spk::SpriteSheet::Sprite> &p_sprites)
	{
		_sprites = p_sprites;
	}

	bool isVoxel() const override
	{
		return (true);
	}

	void insert(spk::VoxelMesh &p_mesh, const spk::Vector3 &p_base, const std::array<bool, 6> &p_visible) const override
	{
		struct FaceData
		{
			spk::Vector3 v0;
			spk::Vector3 v1;
			spk::Vector3 v2;
			spk::Vector3 v3;
		};
		static const std::array<FaceData, 6> faces = {{{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
													   {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
													   {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
													   {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
													   {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
													   {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}}};
		for (std::size_t i = 0; i < faces.size(); ++i)
		{
			if ((p_visible[i] == true))
			{
				const FaceData &f = faces[i];
				_addQuad(p_mesh, _sprites[i], p_base + f.v0, p_base + f.v1, p_base + f.v2, p_base + f.v3);
			}
		}
	}
};

class SlopeBlock : public Block
{
public:
	enum class Orientation
	{
		RToL,
		LToR,
		UToD,
		DToT
	};

	SlopeBlock(Orientation p_orientation, const std::vector<spk::SpriteSheet::Sprite> &p_sprites) :
		_orientation(p_orientation)
	{
		_sprites = p_sprites;
	}

	void insert(spk::VoxelMesh &, const spk::Vector3 &, const std::array<bool, 6> &) const override
	{
		// TODO: Implement slope insertion
	}

private:
	Orientation _orientation;
};

class HalfBlock : public Block
{
public:
	enum class Orientation
	{
		PX,
		NX,
		PY,
		NY,
		PZ,
		NZ
	};

	HalfBlock(Orientation p_orientation, const std::vector<spk::SpriteSheet::Sprite> &p_sprites) :
		_orientation(p_orientation)
	{
		_sprites = p_sprites;
	}

	void insert(spk::VoxelMesh &, const spk::Vector3 &, const std::array<bool, 6> &) const override
	{
		// TODO: Implement half block insertion
	}

private:
	Orientation _orientation;
};

template <size_t X, size_t Y, size_t Z>
class BlockMap
{
public:
	using BlockIndex = int;

	class Chunk
	{
	public:
		static inline const spk::Vector3UInt size = spk::Vector3UInt(X, Y, Z);

	private:
		BlockMap *_blockMap;
		spk::Vector3Int _coordinate;
		std::array<BlockIndex, X * Y * Z> _blockIndices = {};
		spk::VoxelMesh _mesh;

		std::size_t _arrayIndex(std::size_t p_x, std::size_t p_y, std::size_t p_z) const
		{
			return (p_x + X * (p_y + Y * p_z));
		}

		bool _contains(int p_x, int p_y, int p_z) const
		{
			return ((p_x >= 0 && p_x < (int)X && p_y >= 0 && p_y < (int)Y && p_z >= 0 && p_z < (int)Z) == true);
		}

		const Block *_blockAt(int p_x, int p_y, int p_z) const
		{
			if ((_contains(p_x, p_y, p_z) == true))
			{
				BlockIndex index =
					_blockIndices[_arrayIndex(static_cast<std::size_t>(p_x), static_cast<std::size_t>(p_y), static_cast<std::size_t>(p_z))];
				return (_blockMap->block(index));
			}

			int localX = p_x;
			int localY = p_y;
			int localZ = p_z;
			spk::Vector3Int chunkOffset(0, 0, 0);

			if (localX < 0)
			{
				chunkOffset.x = -1;
				localX += static_cast<int>(X);
			}
			else if (localX >= static_cast<int>(X))
			{
				chunkOffset.x = 1;
				localX -= static_cast<int>(X);
			}

			if (localY < 0)
			{
				chunkOffset.y = -1;
				localY += static_cast<int>(Y);
			}
			else if (localY >= static_cast<int>(Y))
			{
				chunkOffset.y = 1;
				localY -= static_cast<int>(Y);
			}

			if (localZ < 0)
			{
				chunkOffset.z = -1;
				localZ += static_cast<int>(Z);
			}
			else if (localZ >= static_cast<int>(Z))
			{
				chunkOffset.z = 1;
				localZ -= static_cast<int>(Z);
			}

			const Chunk *neighbourChunk = _blockMap->chunk(_coordinate + chunkOffset);
			if (neighbourChunk == nullptr)
			{
				return (nullptr);
			}
			return (neighbourChunk->_blockAt(localX, localY, localZ));
		}

	public:
		Chunk(BlockMap *p_blockMap, const spk::Vector3Int &p_coord) :
			_blockMap(p_blockMap),
			_coordinate(p_coord)
		{
			_blockIndices.fill(-1);
		}

		void setBlock(std::size_t p_x, std::size_t p_y, std::size_t p_z, BlockIndex p_index)
		{
			_blockIndices[_arrayIndex(p_x, p_y, p_z)] = p_index;
		}

		void bake()
		{
			_mesh.clear();
			for (std::size_t z = 0; z < Z; ++z)
			{
				for (std::size_t y = 0; y < Y; ++y)
				{
					for (std::size_t x = 0; x < X; ++x)
					{
						BlockIndex index = _blockIndices[_arrayIndex(x, y, z)];
						if ((index == -1))
						{
							continue;
						}
						const Block *block = _blockMap->block(index);
						if ((block == nullptr))
						{
							continue;
						}
						std::array<bool, 6> visible = {};
						static const std::array<spk::Vector3Int, 6> directions = {
							spk::Vector3Int(0, 1, 0),
							spk::Vector3Int(0, -1, 0),
							spk::Vector3Int(-1, 0, 0),
							spk::Vector3Int(1, 0, 0),
							spk::Vector3Int(0, 0, 1),
							spk::Vector3Int(0, 0, -1)
						};
						for (std::size_t i = 0; i < directions.size(); ++i)
						{
							spk::Vector3Int offset = directions[i];
							const Block *neighbour = _blockAt(static_cast<int>(x) + offset.x, static_cast<int>(y) + offset.y, static_cast<int>(z) + offset.z);
							visible[i] = ((neighbour == nullptr || neighbour->isVoxel() == false) == true);
						}
						block->insert(_mesh, spk::Vector3((float)x, (float)y, (float)z), visible);
					}
				}
			}
		}

		const spk::VoxelMesh &mesh() const
		{
			return (_mesh);
		}
	};

private:
	std::unordered_map<spk::Vector3Int, Chunk> _chunks;
	std::unordered_map<BlockIndex, std::unique_ptr<Block>> _blockDefinitions;

public:
	void setBlockDefinition(BlockIndex p_index, std::unique_ptr<Block> p_block)
	{
		_blockDefinitions[p_index] = std::move(p_block);
	}

	const Block *block(BlockIndex p_index) const
	{
		auto it = _blockDefinitions.find(p_index);
		if ((it != _blockDefinitions.end()) == true)
		{
			return (it->second.get());
		}
		return (nullptr);
	}

	typename BlockMap<X, Y, Z>::Chunk &requestChunk(const spk::Vector3Int &p_coord)
	{
		auto it = _chunks.find(p_coord);
		if ((it == _chunks.end()) == true)
		{
			it = _chunks.try_emplace(p_coord, this, p_coord).first;
		}
		return (it->second);
	}

	Chunk *chunk(const spk::Vector3Int &p_coord)
	{
		auto it = _chunks.find(p_coord);
		if ((it != _chunks.end()) == true)
		{
			return (&it->second);
		}
		return (nullptr);
	}

	const Chunk *chunk(const spk::Vector3Int &p_coord) const
	{
		auto it = _chunks.find(p_coord);
		if ((it != _chunks.end()) == true)
		{
			return (&it->second);
		}
		return (nullptr);
	}
};

class CameraComponent : public spk::Component
{
private:
	spk::Camera _camera;
	spk::ContractProvider::Contract _transformContract;

	void _updateUBO()
	{
		auto &cameraUBO = spk::Lumina::Shader::Constants::ubo(L"CameraUBO");
		const spk::Transform &transform = owner()->transform();
		cameraUBO.layout()[L"viewMatrix"] = transform.inverseModel();
		cameraUBO.layout()[L"projectionMatrix"] = _camera.projectionMatrix();
		cameraUBO.validate();
	}

public:
	CameraComponent(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void setPerspective(float p_fov, float p_aspectRatio, float p_near = 0.1f, float p_far = 1000.0f)
	{
		_camera.setPerspective(p_fov, p_aspectRatio, p_near, p_far);
		_updateUBO();
	}

	void start() override
	{
		_transformContract = owner()->transform().addOnEditionCallback([this]() { _updateUBO(); });
		_updateUBO();
	}

	void onGeometryChange(const spk::Geometry2D& p_geometry) override
	{
		setPerspective(60.0f, static_cast<float>(p_geometry.size.x) / static_cast<float>(p_geometry.size.y));
	}

	spk::Camera &camera()
	{
		return _camera;
	}
};

class FPSPlayerController : public spk::Component
{
private:
	float _moveSpeed;
	float _mouseSensitivity;

	bool _isMovingCamera;
	spk::Vector2Int _lastMousePosition;

	std::vector<std::unique_ptr<spk::Action>> _actions;

	spk::Vector3 _motionRequested = {0, 0, 0};
	spk::Vector2Int _rotationRequested = {0, 0};

public:
	FPSPlayerController(const std::wstring &p_name, float p_moveSpeed = 5.0f, float p_mouseSensitivity = 0.1f) :
		spk::Component(p_name),
		_moveSpeed(p_moveSpeed),
		_mouseSensitivity(p_mouseSensitivity)
	{
		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::Z, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested += owner()->transform().forward();
		}));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::Q, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested -= owner()->transform().right();
		}));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::S, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested -= owner()->transform().forward();
		}));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::D, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested += owner()->transform().right();
		}));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::Space, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested += spk::Vector3{0, 1, 0};
		}));

		_actions.push_back(std::make_unique<spk::KeyboardAction>(spk::Keyboard::Control, spk::InputState::Down, 10, [&](const spk::SafePointer<const spk::Keyboard>& p_keyboard){
			_motionRequested -= spk::Vector3{0, 1, 0};
		}));

		_actions.push_back(std::make_unique<spk::MouseButtonAction>(spk::Mouse::Button::Left, spk::InputState::Down, -1, [&](const spk::SafePointer<const spk::Mouse>& p_mouse){
			_isMovingCamera = true;
			_lastMousePosition = p_mouse->position();
		}));
		_actions.push_back(std::make_unique<spk::MouseButtonAction>(spk::Mouse::Button::Left, spk::InputState::Up, -1, [&](const spk::SafePointer<const spk::Mouse>& p_mouse){
			_isMovingCamera = false;
		}));

		_actions.push_back(std::make_unique<spk::MouseMotionAction>(spk::MouseMotionAction::Mode::Absolute, [&](const spk::Vector2Int& p_mousePosition){
			if (_isMovingCamera == false)
			{
				return;
			}
			
			spk::Vector2Int deltaPosition = p_mousePosition - _lastMousePosition;

			if (deltaPosition != spk::Vector2Int(0, 0))
			{
				_rotationRequested.x = static_cast<float>(deltaPosition.x) * _mouseSensitivity;
				_rotationRequested.y = static_cast<float>(deltaPosition.y) * _mouseSensitivity;
			}

			_lastMousePosition = p_mousePosition;
		}));
	}

	void onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		if (p_event.keyboard == nullptr || p_event.mouse == nullptr)
		{
			return;
		}

		_motionRequested = spk::Vector3(0, 0, 0);
		_rotationRequested = spk::Vector2(0, 0);

		for (auto& action : _actions)
		{
			if (action->isInitialized() == false)
			{
				action->initialize(p_event);
			}

			action->update();
		}		

		bool isMotionRequested = _motionRequested != spk::Vector3();
		bool isRotationRequested = _rotationRequested != spk::Vector2();

		if (isMotionRequested == true || isRotationRequested == true)
		{
			if (_motionRequested != spk::Vector3())
			{
				auto delta = _motionRequested.normalize() * (float)p_event.deltaTime.seconds * _moveSpeed;
				owner()->transform().move(delta);
			}

			if (_rotationRequested != spk::Vector2())
			{
				owner()->transform().rotateAroundAxis({0, 1, 0}, _rotationRequested.x);
				owner()->transform().rotateAroundAxis(owner()->transform().right(), _rotationRequested.y);
			}
		
			p_event.requestPaint();
		}
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(1);
	window->requestMousePlacement({400, 300});

	spk::SafePointer<spk::GameEngine> engine = new spk::GameEngine();
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(engine);
	engineWidget.activate();

	spk::SafePointer<spk::Entity> player = new spk::Entity(L"Player");
	player->activate();
	engine->addEntity(player);

	auto cameraComponent = player->addComponent<CameraComponent>(L"Player/CameraComponent");
	cameraComponent->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	auto playerController = player->addComponent<FPSPlayerController>(L"Player/FPSPlayerController", 5.0f, 0.1f);
	player->transform().place({5.0f, 5.0f, 5.0f});
	player->transform().lookAt({0.0f, 0.0f, 0.0f});
	
	spk::SafePointer<spk::Entity> cube = new spk::Entity(L"Cube");
	cube->activate();
	engine->addEntity(cube);
	auto renderer = cube->addComponent<spk::ObjMeshRenderer>(L"Cube/ObjMeshRenderer");

	spk::Image texture = spk::Image("playground/resources/texture/CubeTexture.png");

	spk::ObjMesh cubeMesh;
	cubeMesh.loadFromFile("playground/resources/obj/cube.obj");
	renderer->setTexture(&texture);
	renderer->setMesh(&cubeMesh);
	cube->transform().place({0.0f, 0.0f, 0.0f});
	return app.run();
}
