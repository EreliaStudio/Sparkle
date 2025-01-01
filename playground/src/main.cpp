#include "playground.hpp"

enum class Event
{
	NoEvent,
	PlayerMotionIdle,
	PlayerMotionUp,
	PlayerMotionLeft,
	PlayerMotionDown,
	PlayerMotionRight
};


inline const char* to_string(Event event)
{
    switch (event)
    {
        case Event::NoEvent:          return "NoEvent";
        case Event::PlayerMotionIdle: return "PlayerMotionIdle";
        case Event::PlayerMotionUp:   return "PlayerMotionUp";
        case Event::PlayerMotionLeft: return "PlayerMotionLeft";
        case Event::PlayerMotionDown: return "PlayerMotionDown";
        case Event::PlayerMotionRight:return "PlayerMotionRight";
        default:                      return "UnknownEvent";
    }
}

inline const wchar_t* to_wstring(Event event)
{
    switch (event)
    {
        case Event::NoEvent:          return L"NoEvent";
        case Event::PlayerMotionIdle: return L"PlayerMotionIdle";
        case Event::PlayerMotionUp:   return L"PlayerMotionUp";
        case Event::PlayerMotionLeft: return L"PlayerMotionLeft";
        case Event::PlayerMotionDown: return L"PlayerMotionDown";
        case Event::PlayerMotionRight:return L"PlayerMotionRight";
        default:                      return L"UnknownEvent";
    }
}

std::ostream& operator<<(std::ostream& os, Event event)
{
    return os << to_string(event);
}

std::wostream& operator<<(std::wostream& wos, Event event)
{
    return wos << to_wstring(event);
}

using EventCenter = spk::Singleton<spk::EventNotifier<Event>>;

struct Node
{
	spk::Vector2 animationStartPos;
	int frameDuration;
	int animationLength;
	int animationStep;
};

class Chunk
{
public:
	static inline const size_t Size = 16;
	static inline const size_t Layer = 5;

private:
	int _content[Size][Size][Layer];

public:
	Chunk()
	{
		std::memset(&_content, -1, sizeof(_content));
	}

	void setContent(const spk::Vector3Int& p_relPosition, int p_value)
	{
		_content[p_relPosition.x][p_relPosition.y][p_relPosition.z] = p_value;
	}

	void setContent(const spk::Vector2Int& p_relPosition, const int& p_layer, int p_value)
	{
		_content[p_relPosition.x][p_relPosition.y][p_layer] = p_value;
	}

	void setContent(const int& p_x, const int& p_y, const int& p_z, int p_value)
	{
		_content[p_x][p_y][p_z] = p_value;
	}

	int content(const spk::Vector3Int& p_relPosition) const
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_relPosition.z]);
	}

	int content(const spk::Vector2Int& p_relPosition, const int& p_layer) const
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_layer]);
	}

	int content(const int& p_x, const int& p_y, const int& p_z) const
	{
		return (_content[p_x][p_y][p_z]);
	}
};

class BufferObjectCollection : public spk::Singleton<BufferObjectCollection>
{
friend class spk::Singleton<BufferObjectCollection>;
private:
	std::unordered_map<std::string, spk::OpenGL::BindedBufferObject*> _bindedBufferObjects;

	BufferObjectCollection()
	{
		spk::OpenGL::ShaderStorageBufferObject& nodeMapSSBO = allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 4, 4, 20, 4);

		nodeMapSSBO.addElement("nbNodes", 0, 4);

		nodeMapSSBO["nbNodes"] = 0;

		nodeMapSSBO.validate();

		spk::OpenGL::UniformBufferObject& cameraUBO = allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 1, 128);
			
		cameraUBO.addElement("view", 0, 64);
		cameraUBO.addElement("projection", 64, 64);

		cameraUBO["view"] = spk::Matrix4x4::identity();
		cameraUBO["projection"] = spk::Matrix4x4::identity();

		cameraUBO.validate();

		spk::OpenGL::UniformBufferObject& chunkTextureInfo = allocate<spk::OpenGL::UniformBufferObject>("chunkTextureInfo", "ChunkTextureInfo_Type", 5, 8);

		chunkTextureInfo.addElement("unit", 0, 8);

		chunkTextureInfo["unit"] = spk::Vector2(1, 1);

		chunkTextureInfo.validate();

		spk::OpenGL::UniformBufferObject& systemInfo = allocate<spk::OpenGL::UniformBufferObject>("systemInfo", "SystemInfo_Type", 6, 4);

		systemInfo.addElement("time", 0, 4);

		systemInfo["time"] = 0;

		systemInfo.validate();
	}

public:
	template<typename TBindedObject, typename... TArgs>
	TBindedObject& allocate(const std::string& p_name, TArgs... p_args)
	{
		if (_bindedBufferObjects.contains(p_name))
		{
			throw std::runtime_error("Binding point already allocated");
		}
		TBindedObject* object = new TBindedObject(p_args...);
		_bindedBufferObjects[p_name] = object;
		return (*object); 
	}

	bool isAllocated(const std::string& p_name)
	{
		return (_bindedBufferObjects.contains(p_name));
	}

	spk::OpenGL::UniformBufferObject& UBO(const std::string& p_uniformName)
	{
		if (_bindedBufferObjects.contains(p_uniformName) == false)
		{
			throw std::runtime_error("UBO [" + p_uniformName + "] not found");
		}

		spk::OpenGL::UniformBufferObject* pointer = dynamic_cast<spk::OpenGL::UniformBufferObject*>(_bindedBufferObjects[p_uniformName]);
		
		if (pointer == nullptr)
		{
			throw std::runtime_error("UBO [" + p_uniformName + "] doesn't exist : [" + p_uniformName + "] seem to be a SSBO");
		}

		return (*(pointer));
	}

	spk::OpenGL::ShaderStorageBufferObject& SSBO(const std::string& p_uniformName)
	{
		if (_bindedBufferObjects.contains(p_uniformName) == false)
		{
			throw std::runtime_error("SSBO [" + p_uniformName + "] not found");
		}

		spk::OpenGL::ShaderStorageBufferObject* pointer = dynamic_cast<spk::OpenGL::ShaderStorageBufferObject*>(_bindedBufferObjects[p_uniformName]);
		
		if (pointer == nullptr)
		{
			throw std::runtime_error("SSBO [" + p_uniformName + "] doesn't exist : [" + p_uniformName + "] seem to be a UBO");
		}

		return (*(pointer));
	}
};

class NodeMap
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::ShaderStorageBufferObject& _nodeMapSSBO;
	std::vector<Node> _content;

public:
	NodeMap() :
		_nodeMapSSBO(BufferObjectCollection::instance()->SSBO("nodeConstants"))
	{

	}

	void addNode(const int& p_id, const Node& p_node)
	{
		if (p_id == -1)
		{
			throw std::runtime_error("Invalid node ID");
		}
		if (p_id >= _content.size())
		{
			_content.resize(p_id + 1);
		}
		_content[p_id] = p_node;
	}

	void validate()
	{
		if (_content.size() == 0)
		{
			throw std::runtime_error("Node map is empty");
		}
		
		_nodeMapSSBO["nbNodes"] = int(_content.size());
		_nodeMapSSBO.resizeDynamicArray(_content.size());
		_nodeMapSSBO.dynamicArray() = _content;
		_nodeMapSSBO.validate();
	}

	const Node& operator[](const int& p_id) const
	{
		if (p_id == -1)
		{
			throw std::runtime_error("Invalid node ID");
		}
		return (_content[p_id]);
	}
};

class BakableChunk : public Chunk
{
private:
	bool _needBake = true;

public:
	BakableChunk() :
		Chunk()
	{
		
	}

	bool needBake()
	{
		return (_needBake);
	}

	void invalidate()
	{
		_needBake = true;
	}

	void validate()
	{
		_needBake = false;
	}
};

class ChunkRenderer
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;

	static inline spk::OpenGL::Program* _program = nullptr;
	spk::OpenGL::UniformBufferObject& _cameraUBO;
	spk::OpenGL::UniformBufferObject& _chunkTextureInfo;
	spk::OpenGL::UniformBufferObject& _systemInfo;
	spk::OpenGL::ShaderStorageBufferObject& _nodeMapSSBO;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::UniformBufferObject _transformUBO;
	spk::OpenGL::ShaderStorageBufferObject _cellListSSBO;
	spk::OpenGL::ShaderStorageBufferObject _chunkSSBO;
	spk::OpenGL::SamplerObject _spriteSheetSampler;

	void _initProgram()
	{
		if (_program != nullptr)
		{
			return;
		}

		std::string vertexShaderSrc = spk::FileUtils::readFileAsString("playground/resources/shader/chunkShader.vert");

		std::string fragmentShaderSrc = spk::FileUtils::readFileAsString("playground/resources/shader/chunkShader.frag");

		_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
	}

	void _initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2} // inCellIndex
		});

		_bufferSet.clear();

		_bufferSet.layout() << spk::Vector2(0, 0);
		_bufferSet.layout() << spk::Vector2(1, 0);
		_bufferSet.layout() << spk::Vector2(0, 1);
		_bufferSet.layout() << spk::Vector2(1, 1);

		_bufferSet.indexes() << 0 << 1 << 2 << 2 << 1 << 3;

		_bufferSet.validate();

		_transformUBO = spk::OpenGL::UniformBufferObject("Transform_Type", 0, 64);
		_transformUBO.addElement("model", 0, 64);

		_transformUBO["model"] = spk::Matrix4x4::identity();

		_transformUBO.validate();

		_cellListSSBO = spk::OpenGL::ShaderStorageBufferObject("CellList_Type", 2, 0, 0, 12, 4);

		_chunkSSBO = spk::OpenGL::ShaderStorageBufferObject("Chunk_Type", 3, 5120, 0, 0, 0);

		_chunkSSBO.addElement("content", 0, 4, 1280, 0);

		_chunkSSBO["content"] = std::vector<int>(16 * 16 * 5, -1);

		_chunkSSBO.validate();

		_spriteSheetSampler = spk::OpenGL::SamplerObject("spriteSheet", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
	}

public:
	ChunkRenderer() :
		_cameraUBO(BufferObjectCollection::instance()->UBO("camera")),
		_nodeMapSSBO(BufferObjectCollection::instance()->SSBO("nodeConstants")),
		_chunkTextureInfo(BufferObjectCollection::instance()->UBO("chunkTextureInfo")),
		_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo"))
	{
		_initProgram();
		_initBuffers();
	}

	void updateSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_spriteSheetSampler.bind(p_spriteSheet);
		_chunkTextureInfo["unit"] = p_spriteSheet->unit();
		_chunkTextureInfo.validate();
	}	

	void updateChunk(spk::SafePointer<const BakableChunk> p_chunk)
	{
		_chunkSSBO["content"] = *(dynamic_cast<const Chunk*>(p_chunk.get()));

		_chunkSSBO.validate();
	}

	void updateTransform(spk::Transform& p_transform)
	{
		_transformUBO["model"] = p_transform.model();

		_transformUBO.validate();
	}

	void updateChunkData(spk::SafePointer<const Chunk> p_chunk)
	{
		std::vector<spk::Vector3Int> cellList;

		for (size_t i = 0; i < Chunk::Size; i++)
		{
			for (size_t j = 0; j < Chunk::Size; j++)
			{
				for (size_t k = 0; k < Chunk::Layer; k++)
				{
					spk::Vector3Int tmp = spk::Vector3Int(i, j, k);

					if (p_chunk->content(tmp) != -1)
					{
						cellList.push_back(tmp);
					}
				}
			}
		}

		_cellListSSBO.resizeDynamicArray(cellList.size());

		_cellListSSBO.dynamicArray() = cellList;

		_cellListSSBO.validate();
	}

	void render()
	{
		_program->activate();

		_systemInfo.activate();
		_spriteSheetSampler.activate();
		_chunkSSBO.activate();
		_chunkTextureInfo.activate();
		_nodeMapSSBO.activate();
		_cellListSSBO.activate();
		_cameraUBO.activate();
		_transformUBO.activate();
		_bufferSet.activate();
		
		_program->render(_bufferSet.indexes().nbIndexes(), _cellListSSBO.dynamicArray().nbElement());

		_bufferSet.deactivate();
		_transformUBO.deactivate();
		_cameraUBO.deactivate();
		_cellListSSBO.deactivate();
		_nodeMapSSBO.deactivate();
		_chunkTextureInfo.deactivate();
		_chunkSSBO.deactivate();
		_spriteSheetSampler.deactivate();
		_systemInfo.deactivate();

		_program->deactivate();
	}
};

class ChunkComponent : public spk::Component
{
private:
	spk::SafePointer<BakableChunk> _chunk;
	ChunkRenderer _renderer;

	spk::Entity::Contract _onEditionContract;

public:
	ChunkComponent() :
		spk::Component(L"ChunkComponent")
	{

	}

	void setChunk(spk::SafePointer<BakableChunk> p_chunk)
	{
		_chunk = p_chunk;
	}

	void setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_renderer.updateSpriteSheet(p_spriteSheet);
	}

	void start() override
	{
		_onEditionContract = owner()->transform().addOnEditionCallback([&](){
			_renderer.updateTransform(owner()->transform());
		});
		_onEditionContract.trigger();
	}

	void onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (_chunk->needBake() == true)
		{
			_renderer.updateChunk(_chunk);
			_renderer.updateChunkData(_chunk);
			_chunk->validate();
		}

		_renderer.render();
	}
};

class CameraComponent : public spk::Component
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::UniformBufferObject& _cameraUBO;

	spk::Camera _camera;

	spk::Entity::Contract _onEditionContract;

public:
	CameraComponent(const std::wstring& p_name) :
		spk::Component(p_name),
		_cameraUBO(BufferObjectCollection::instance()->UBO("camera"))
	{
		setPerspective(90, 1, 0.1f, 1000.0f);
		// setOrthographic(-20, 20, -20, 20);
	}

	void start() override
	{
		_onEditionContract = owner()->transform().addOnEditionCallback([&](){
			_cameraUBO["view"] = owner()->transform().model();
		});
		
		_onEditionContract.trigger();
	}

	void setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane = 0.1f, float p_farPlane = 1000.0f)
	{
		_camera.setPerspective(p_fovDegrees, p_aspectRatio, p_nearPlane, p_farPlane);
		_cameraUBO["projection"] = _camera.projectionMatrix();
	}
        
	void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f)
	{
		_camera.setOrthographic(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
		_cameraUBO["projection"] = _camera.projectionMatrix();
	}
};

struct WorldManagerComponent : public spk::Component
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::UniformBufferObject& _systemInfo;

public:
	WorldManagerComponent(const std::wstring& p_name) :
		spk::Component(p_name),
		_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo"))
	{

	}

	void onUpdateEvent(spk::UpdateEvent& p_event) override
	{
		if (p_event.deltaTime.milliseconds != 0)
		{
			_systemInfo["time"] = static_cast<int>(p_event.time.milliseconds);
			_systemInfo.validate();

			p_event.requestPaint();
		}
	}
};

class ControlMapper : public spk::Component
{
private:

	EventCenter::Instanciator _eventCenterInstanciator;

	Event _motionEvent = Event::NoEvent;

public:
	ControlMapper(const std::wstring& p_name) :
		spk::Component(p_name)
	{

	}

	void onControllerEvent(spk::ControllerEvent& p_event) override
	{
		if (p_event.type == spk::ControllerEvent::Type::JoystickMotion)
		{
			if (p_event.joystick.id == spk::Controller::Joystick::Right)
				return ;
			_motionEvent = Event::NoEvent;

			if (std::abs(p_event.controller->leftJoystick.position.x) > std::abs(p_event.controller->leftJoystick.position.y))
			{
				if (p_event.controller->leftJoystick.position.x < -100)
				{
					_motionEvent = Event::PlayerMotionLeft;
				}
				else if (p_event.controller->leftJoystick.position.x > 100)
				{
					_motionEvent = Event::PlayerMotionRight;
				}
			}
			else
			{
				if (p_event.controller->leftJoystick.position.y < -100)
				{
					_motionEvent = Event::PlayerMotionDown;
				}
				else if (p_event.controller->leftJoystick.position.y > 100)
				{
					_motionEvent = Event::PlayerMotionUp;
				}
			}
		}
		else if (p_event.type == spk::ControllerEvent::Type::JoystickReset)
		{
			if (p_event.joystick.id == spk::Controller::Joystick::Right)
				return ;
				
			_motionEvent = Event::PlayerMotionIdle;
		}
	}

	void onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press ||
			p_event.type == spk::KeyboardEvent::Type::Release)
		{
			_motionEvent = Event::PlayerMotionIdle;

			if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::Z)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionUp;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::Q)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionLeft;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::S)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionDown;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::D)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionRight;
			}
		}
	}

	void onMouseEvent(spk::MouseEvent& p_event) override
	{

	}

	void onUpdateEvent(spk::UpdateEvent& p_event) override
	{
		static Event _lastEvent = Event::NoEvent;

		if (_motionEvent != _lastEvent)
		{
			EventCenter::instance()->notifyEvent(_motionEvent);
			_lastEvent = _motionEvent;
		}
	}

};

class PlayerController : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;

	EventCenter::Type::Contract _upMotionContract;
	EventCenter::Type::Contract _leftMotionContract;
	EventCenter::Type::Contract _downMotionContract;
	EventCenter::Type::Contract _rightMotionContract;
	EventCenter::Type::Contract _idleMotionContract;

	spk::Timer _motionTimer;
	spk::Vector3 _direction;
	spk::Vector3 _origin; // Express in unit per millisecond
	spk::Vector3 _destination; // Express in unit per millisecond

	void movePlayer(const spk::Vector3& p_direction)
	{
		_direction = p_direction;

		if (_motionTimer.state() == spk::Timer::State::Running)
		{
			return;
		}

		_origin = owner()->transform().localPosition();
		_destination = _origin + p_direction;
		_motionTimer.start();
	}

	void stopPlayer()
	{
		_direction = 0;
	}

public:
	PlayerController(const std::wstring& p_name) :
		spk::Component(p_name),
		_motionTimer(150LL, spk::TimeUnit::Millisecond),
		_upMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionUp, [&](){
			movePlayer(spk::Vector3(0, 1, 0));
		})),
		_leftMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionLeft, [&](){
			movePlayer(spk::Vector3(-1, 0, 0));
		})),
		_downMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionDown, [&](){
			movePlayer(spk::Vector3(0, -1, 0));
		})),
		_rightMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionRight, [&](){
			movePlayer(spk::Vector3(1, 0, 0));
		})),
		_idleMotionContract(EventCenter::instance()->subscribe(Event::PlayerMotionIdle, [&](){
			stopPlayer();
		}))
	{

	}

	void onUpdateEvent(spk::UpdateEvent& p_event) override
	{
		if (p_event.deltaTime.milliseconds == 0)
		{
			return;
		}

		if (_motionTimer.state() != spk::Timer::State::Running)
		{
			if (_origin != _destination)
			{
				if (_direction == 0)
				{
					stopPlayer();
				}
				else
				{
					movePlayer(_direction);
				}
			}
			return;
		} 

		if (_origin != _destination)
		{
			float ratio = static_cast<double>(_motionTimer.elapsed().value) / static_cast<double>(_motionTimer.expectedDuration().value);
			owner()->transform().place(spk::Vector3::lerp(_origin, _destination, ratio));
		}
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	spk::SpriteSheet chunkSpriteSheet = spk::SpriteSheet(L"playground/resources/test.png", spk::Vector2UInt(2, 2));

	NodeMap nodeMap;

	nodeMap.addNode(0, {
	 	.animationStartPos = spk::Vector2(0, 0),
		.frameDuration = 1000,
		.animationLength = 2,
		.animationStep = 1		
	});

	nodeMap.addNode(1, {
	 	.animationStartPos = spk::Vector2(1, 1),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.addNode(2, {
	 	.animationStartPos = spk::Vector2(1, 0),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.addNode(3, {
	 	.animationStartPos = spk::Vector2(0, 1),
		.frameDuration = 100,
		.animationLength = 0,
		.animationStep = 1		
	});

	nodeMap.validate();

	BakableChunk chunk[4];

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t x = 0; x < Chunk::Size; x++)
		{
			for (size_t y = 0; y < Chunk::Size; y++)
			{
				for (size_t z = 0; z < Chunk::Layer; z++)
				{
					if (z == 0 && (
						x == 0 || x == Chunk::Size - 1 || 
						y == 0 || y == Chunk::Size - 1))
					{
						chunk[i].setContent(x, y, z, 0);
					}
					else if (z == 1 && (
						x == 1 || x == Chunk::Size - 2 || 
						y == 1 || y == Chunk::Size - 2))
					{
						chunk[i].setContent(x, y, z, 1);
					}
					else if (z == 2 && (
						x == 2 || x == Chunk::Size - 3 || 
						y == 2 || y == Chunk::Size - 3))
					{
						chunk[i].setContent(x, y, z, 2);
					}
					else if (z == 3 && (
						x == 3 || x == Chunk::Size - 4 || 
						y == 3 || y == Chunk::Size - 4))
					{
						chunk[i].setContent(x, y, z, 3);
					}
				}
			}
		}	
		
		chunk[i].invalidate();
	}


	spk::GameEngine engine;

	spk::Entity worldManager = spk::Entity(L"World manager");
	worldManager.addComponent<WorldManagerComponent>(L"World manager component");
	worldManager.addComponent<ControlMapper>(L"Control mapper component");

	spk::Entity player = spk::Entity(L"Player");
	player.addComponent<PlayerController>(L"Player controler component");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, -20));

	CameraComponent& cameraComp = camera.addComponent<CameraComponent>(L"Main camera");

	spk::Entity chunkObject[4] = {
		spk::Entity(L"Chunk A"),
		spk::Entity(L"Chunk B"),
		spk::Entity(L"Chunk C"),
		spk::Entity(L"Chunk D")
	};

	for (int x = 0; x < 2; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			chunkObject[x + y * 2].transform().place(spk::Vector3((x - 1) * static_cast<int>(Chunk::Size), (y - 1) * static_cast<int>(Chunk::Size), 0));
			ChunkComponent& chunkComp = chunkObject[x + y * 2].addComponent<ChunkComponent>();
			chunkComp.setChunk(&chunk[x + y * 2]);
			chunkComp.setSpriteSheet(&chunkSpriteSheet);
		}	
	}


	engine.addEntity(&worldManager);
	engine.addEntity(&player);
	for (size_t i = 0; i < 4; i++)
	{
		engine.addEntity(&chunkObject[i]);
	}

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}