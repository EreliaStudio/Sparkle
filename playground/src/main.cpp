#include "playground.hpp"

#include <fstream>

enum class Event
{
	NoEvent,
	PlayerMotionIdle,
	PlayerMotionUp,
	PlayerMotionLeft,
	PlayerMotionDown,
	PlayerMotionRight,
	UpdateChunkVisibility
};

inline const char* to_string(Event event)
{
    switch (event)
    {
        case Event::NoEvent:            return "NoEvent";
        case Event::PlayerMotionIdle:   return "PlayerMotionIdle";
        case Event::PlayerMotionUp:     return "PlayerMotionUp";
        case Event::PlayerMotionLeft:   return "PlayerMotionLeft";
        case Event::PlayerMotionDown:   return "PlayerMotionDown";
        case Event::PlayerMotionRight:  return "PlayerMotionRight";
        case Event::UpdateChunkVisibility: return "UpdateChunkVisibility";
        default:                        return "UnknownEvent";
    }
}

inline const wchar_t* to_wstring(Event event)
{
    switch (event)
    {
        case Event::NoEvent:            return L"NoEvent";
        case Event::PlayerMotionIdle:   return L"PlayerMotionIdle";
        case Event::PlayerMotionUp:     return L"PlayerMotionUp";
        case Event::PlayerMotionLeft:   return L"PlayerMotionLeft";
        case Event::PlayerMotionDown:   return L"PlayerMotionDown";
        case Event::PlayerMotionRight:  return L"PlayerMotionRight";
        case Event::UpdateChunkVisibility: return L"UpdateChunkVisibility";
        default:                        return L"UnknownEvent";
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

class TextureManager : public spk::Singleton<TextureManager>
{
	friend class spk::Singleton<TextureManager>;
private:
	std::unordered_map<std::wstring, std::unique_ptr<spk::OpenGL::TextureObject>> _textures;

	TextureManager()
	{
		loadSpriteSheet(L"chunkSpriteSheet", new spk::SpriteSheet(L"playground/resources/test.png", spk::Vector2UInt(2, 2)));
	}

public:
	spk::SafePointer<spk::Image> loadImage(const std::wstring& p_name, spk::Image* p_image)
	{
		if (_textures.contains(p_name) == true)
		{
			throw std::runtime_error("Texture already loaded");
		}
		_textures[p_name] = std::unique_ptr<spk::Image>(p_image);
		spk::Image* convertedPtr = dynamic_cast<spk::Image*>(_textures[p_name].get());
		return (spk::SafePointer<spk::Image>(convertedPtr));
	}
	
	spk::SafePointer<spk::SpriteSheet> loadSpriteSheet(const std::wstring& p_name, spk::SpriteSheet* p_spriteSheet)
	{
		if (_textures.contains(p_name) == true)
		{
			throw std::runtime_error("Texture already loaded");
		}
		_textures[p_name] = std::unique_ptr<spk::SpriteSheet>(p_spriteSheet);
		spk::SpriteSheet* convertedPtr = dynamic_cast<spk::SpriteSheet*>(_textures[p_name].get());
		return (spk::SafePointer<spk::SpriteSheet>(convertedPtr));
	}

	bool contain(const std::wstring& p_name)
	{
		return (_textures.contains(p_name));
	}
	
	spk::SafePointer<spk::Image> image(const std::wstring& p_name)
	{
		if (!_textures.contains(p_name))
		{
			throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] not found");
		}

		spk::Image* convertedPtr = dynamic_cast<spk::Image*>(_textures[p_name].get());

		if (convertedPtr == nullptr)
		{
			throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] is not a Image");
		}
		return (convertedPtr);
	}
	
	spk::SafePointer<spk::SpriteSheet> spriteSheet(const std::wstring& p_name)
	{
		if (!_textures.contains(p_name))
		{
			throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] not found");
		}

		spk::SpriteSheet* convertedPtr = dynamic_cast<spk::SpriteSheet*>(_textures[p_name].get());

		if (convertedPtr == nullptr)
		{
			throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] is not a SpriteSheet");
		}

		return (spk::SafePointer<spk::SpriteSheet>(convertedPtr));
	}

};

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

	static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector2& p_worldPosition)
	{
		return (spk::Vector2Int(
			std::floor(p_worldPosition.x / static_cast<float>(Size)), 
			std::floor(p_worldPosition.y / static_cast<float>(Size)))
		);
	}

	static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector3& p_worldPosition)
	{
		return (spk::Vector2Int(
			std::floor(p_worldPosition.x / static_cast<float>(Size)), 
			std::floor(p_worldPosition.y / static_cast<float>(Size)))
		);
	}

	static spk::Vector3 convertChunkToWorldPosition(const spk::Vector2Int& p_chunkPosition)
	{
		return (spk::Vector3(p_chunkPosition.x * static_cast<int>(Size), p_chunkPosition.y * static_cast<int>(Size), 0));
	}

	void serialize(std::ofstream& p_os) const
	{
		p_os.write(reinterpret_cast<const char*>(_content), sizeof(_content));
	}

	void deserialize(std::ifstream& p_is)
	{
		p_is.read(reinterpret_cast<char*>(_content), sizeof(_content));
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
		_bufferSet.layout() << spk::Vector2(0, 1);
		_bufferSet.layout() << spk::Vector2(1, 0);
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
		if (_spriteSheetSampler.texture() == nullptr)
		{
			throw std::runtime_error("Chunk sprite sheet not set");
		}

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
		setOrthographic(-20, 20, -20, 20);
	}

	void start() override
	{
		_onEditionContract = owner()->transform().addOnEditionCallback([&](){
			_cameraUBO["view"] = owner()->transform().model();
		});
		
		_onEditionContract.trigger();
	}

	void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f)
	{
		_camera.setOrthographic(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
		_cameraUBO["projection"] = _camera.projectionMatrix();
	}

	const spk::Camera& camera() const
	{
		return (_camera);
	}

	void onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (p_event.type == spk::PaintEvent::Type::Resize)
		{
			spk::Vector2Int cellSize = 32;
			spk::Vector2 cellCount = p_event.geometry.size / cellSize;

			setOrthographic(cellCount.x / -2, cellCount.x / 2, cellCount.y / -2, cellCount.y / 2);
			EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
		}
	}
};

struct ChunkEntity : public spk::Entity
{
private:
	static inline const std::filesystem::path _chunkFolderPath = "playground/resources/chunks/";

	ChunkComponent& _chunkComponent;
	spk::Vector2Int _chunkPosition;
	BakableChunk _chunk;

	void _loadChunk(const spk::Vector2Int& p_chunkPosition)
	{
		std::filesystem::path inputFilePath = _chunkFolderPath.string() + "chunk_" + std::to_string(p_chunkPosition.x) + "_" + std::to_string(p_chunkPosition.y) + ".chunk";
		
		std::ifstream file(inputFilePath, std::ios::binary);

		if (file.is_open() == true)
		{
			_chunk.deserialize(file);
		}
		else
		{
			bool isEven = (p_chunkPosition.x % 2 == 0 && p_chunkPosition.y % 2 == 0) || (p_chunkPosition.x % 2 != 0 && p_chunkPosition.y % 2 != 0);
			for (size_t i = 0; i < Chunk::Size; i++)
			{
				for (size_t j = 0; j < Chunk::Size; j++)
				{
					_chunk.setContent(spk::Vector3Int(i, j, 0), (isEven == true ? 0 : 1));

					if (i == spk::positiveModulo(p_chunkPosition.x, Chunk::Size) && j == spk::positiveModulo(p_chunkPosition.y, Chunk::Size))
					{
						_chunk.setContent(spk::Vector3Int(i, j, 0), 2);
					}
				}
			}

			_chunk.setContent(spk::Vector3Int(3, 3, 1), 3);
			_chunk.setContent(spk::Vector3Int(6, 3, 1), 3);
			_chunk.setContent(spk::Vector3Int(3, 6, 1), 3);
			_chunk.setContent(spk::Vector3Int(6, 6, 1), 3);
		}

		_chunk.invalidate();
	}

public:
	ChunkEntity(const spk::Vector2Int& p_chunkPosition, spk::SafePointer<spk::Entity> p_owner) :
		spk::Entity(L"Chunk " + std::to_wstring(p_chunkPosition.x) + L" " + std::to_wstring(p_chunkPosition.y), p_owner),
		_chunkComponent(addComponent<ChunkComponent>()),
		_chunkPosition(p_chunkPosition)
	{
		transform().place(Chunk::convertChunkToWorldPosition(p_chunkPosition));
		_loadChunk(p_chunkPosition);
		_chunkComponent.setChunk(&_chunk);
		_chunkComponent.setSpriteSheet(TextureManager::instance()->spriteSheet(L"chunkSpriteSheet"));
	}

	spk::SafePointer<Chunk> chunk()
	{
		return (&_chunk);
	}

	spk::Vector2Int chunkPosition()
	{
		return (_chunkPosition);
	}
};

struct WorldManagerComponent : public spk::Component
{
private:
	EventCenter::Instanciator _eventCenterInstanciator;
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::UniformBufferObject& _systemInfo;

	spk::ContractProvider::Contract _updateChunkVisibilityContract;

	spk::Vector2Int _halfSize;

	spk::SafePointer<const spk::Entity> _cameraEntity = nullptr;
	spk::SafePointer<const spk::Camera> _camera = nullptr;

	std::filesystem::path _chunkFolderPath;

	std::unordered_map<spk::Vector2Int, std::unique_ptr<ChunkEntity>> _chunkEntities;
	std::vector<spk::SafePointer<ChunkEntity>> _activeChunkList;

	void _updateChunkVisibility()
	{
		// if (_camera == nullptr || _cameraEntity == nullptr)
		// {
		// 	return;
		// }

		// spk::Vector2Int currentCameraChunk = Chunk::convertWorldToChunkPosition(_cameraEntity->transform().position());

		// std::vector<spk::SafePointer<ChunkEntity>> chunkToActivate;

		// spk::Matrix4x4 inverseMatrix = _camera->inverseProjectionMatrix();

		// spk::Vector3 downLeftWorld = inverseMatrix * spk::Vector3(-1, -1, 0);
		// spk::Vector3 topRightWorld = inverseMatrix * spk::Vector3(1, 1, 0);

		// spk::Vector2Int downLeftChunk = Chunk::convertWorldToChunkPosition(downLeftWorld) + currentCameraChunk;
		// spk::Vector2Int topRightChunk = Chunk::convertWorldToChunkPosition(topRightWorld) + currentCameraChunk;

		// spk::cout << "Down left chunk : " << downLeftChunk << std::endl;
		// spk::cout << "Top right chunk : " << topRightChunk << std::endl;

		// for (int x = downLeftChunk.x; x <= topRightChunk.x; x++)
		// {
		// 	for (int y = downLeftChunk.y; y <= topRightChunk.y; y++)
		// 	{
		// 		spk::Vector2Int tmp = spk::Vector2Int(x, y);

		// 		if (_chunkEntities.contains(tmp) == false)
		// 		{
		// 			spk::cout << "Creating chunk [" << tmp << "]" << std::endl;
		// 			_chunkEntities.emplace(tmp, std::make_unique<ChunkEntity>(tmp, owner()));
		// 			_chunkEntities[tmp]->deactivate();
		// 		}

		// 		chunkToActivate.push_back(_chunkEntities[tmp].get());
		// 	}
		// }

		// for (auto& chunk : _activeChunkList)
		// {
		// 	if (std::find(chunkToActivate.begin(), chunkToActivate.end(), chunk) == chunkToActivate.end())
		// 	{
		// 		spk::cout << "Deactivating chunk [" << chunk->name() << "]" << std::endl;
		// 		chunk->deactivate();
		// 	}
		// }

		// _activeChunkList = chunkToActivate;
		
		// for (auto& chunk : _activeChunkList)
		// {
		// 	// spk::cout << "Checking chunk [" << chunk->name() << "]" << std::endl;
		// 	// if (chunk->isActive() == false)
		// 	// {
		// 		spk::cout << "Activating chunk [" << chunk->name() << "]" << std::endl;
		// 		chunk->activate();
		// 	// }
		// }
	}

public:
	WorldManagerComponent(const std::wstring& p_name) :
		spk::Component(p_name),
		_systemInfo(BufferObjectCollection::instance()->UBO("systemInfo")),
		_updateChunkVisibilityContract(EventCenter::instance()->subscribe(Event::UpdateChunkVisibility, [&](){
			_updateChunkVisibility();
		}))
	{

	}

	void awake() override
	{
		EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
		_chunkEntities.emplace(spk::Vector2Int(0, 0), std::make_unique<ChunkEntity>(spk::Vector2Int(0, 0), owner()));
		_chunkEntities.emplace(spk::Vector2Int(1, 1), std::make_unique<ChunkEntity>(spk::Vector2Int(1, 1), owner()));
		_chunkEntities.emplace(spk::Vector2Int(2, 2), std::make_unique<ChunkEntity>(spk::Vector2Int(2, 2), owner()));
		_chunkEntities.emplace(spk::Vector2Int(3, 3), std::make_unique<ChunkEntity>(spk::Vector2Int(3, 3), owner()));
		_chunkEntities.emplace(spk::Vector2Int(4, 4), std::make_unique<ChunkEntity>(spk::Vector2Int(4, 4), owner()));
		_chunkEntities.emplace(spk::Vector2Int(5, 5), std::make_unique<ChunkEntity>(spk::Vector2Int(5, 5), owner()));
	}

	void setCamera(spk::SafePointer<const spk::Entity> p_camera)
	{
		_cameraEntity = p_camera;
		auto& _cameraComponent = _cameraEntity->getComponent<CameraComponent>();
		_camera = &(_cameraComponent.camera());
		if (_camera == nullptr)
		{
			throw std::runtime_error("Camera component not found");
		}

		EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
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

	void onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (p_event.type == spk::PaintEvent::Type::Resize)
		{
			_halfSize = p_event.geometry.size / 2;

			for (auto& chunk : _activeChunkList)
			{
				chunk->deactivate();
			}
			_activeChunkList.clear();
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

		if (Chunk::convertWorldToChunkPosition(_destination) != Chunk::convertWorldToChunkPosition(_origin))
		{
			EventCenter::instance()->notifyEvent(Event::UpdateChunkVisibility);
		}
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
			if (_motionTimer.expectedDuration().value == 0)
			{
				owner()->transform().place(_destination);
				p_event.requestPaint();
			}
			else
			{
				float ratio = static_cast<double>(_motionTimer.elapsed().value) / static_cast<double>(_motionTimer.expectedDuration().value);
				owner()->transform().place(spk::Vector3::lerp(_origin, _destination, ratio));
				p_event.requestPaint();
			}
		}
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	TextureManager::instanciate();
	
	NodeMap nodeMap;

	nodeMap.addNode(0, {
	 	.animationStartPos = spk::Vector2(0, 0),
		.frameDuration = 1000,
		.animationLength = 0,
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

	spk::GameEngine engine;

	spk::Entity worldManager = spk::Entity(L"World manager");
	auto& worldManagerComponent = worldManager.addComponent<WorldManagerComponent>(L"World manager component");
	worldManager.addComponent<ControlMapper>(L"Control mapper component");

	spk::Entity player = spk::Entity(L"Player");
	player.addComponent<PlayerController>(L"Player controler component");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, 20));
	camera.transform().lookAt(spk::Vector3(0, 0, 0));

	CameraComponent& cameraComp = camera.addComponent<CameraComponent>(L"Main camera");

	worldManagerComponent.setCamera(&camera);

	engine.addEntity(&worldManager);
	engine.addEntity(&player);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}