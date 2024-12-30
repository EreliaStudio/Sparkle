#include "playground.hpp"

int count = 0;

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

	}

public:
	template<typename TBindedObject, typename... TArgs>
	spk::SafePointer<TBindedObject> allocate(const std::string& p_name, TArgs... p_args)
	{
		if (_bindedBufferObjects.contains(p_name))
		{
			throw std::runtime_error("Binding point already allocated");
		}
		TBindedObject* object = new TBindedObject(p_args...);
		_bindedBufferObjects[p_name] = object;
		return (object); 
	}

	bool isAllocated(const std::string& p_name)
	{
		return (_bindedBufferObjects.contains(p_name));
	}

	spk::OpenGL::UniformBufferObject* UBO(const std::string& p_uniformName)
	{
		if (_bindedBufferObjects.contains(p_uniformName) == false)
		{
			throw std::runtime_error("UBO [" + p_uniformName + "] not found");
		}

		return (dynamic_cast<spk::OpenGL::UniformBufferObject*>(_bindedBufferObjects[p_uniformName]));
	}

	spk::OpenGL::ShaderStorageBufferObject* SSBO(const std::string& p_uniformName)
	{
		if (_bindedBufferObjects.contains(p_uniformName) == false)
		{
			throw std::runtime_error("SSBO [" + p_uniformName + "] not found");
		}

		return (dynamic_cast<spk::OpenGL::ShaderStorageBufferObject*>(_bindedBufferObjects[p_uniformName]));
	}
};

class NodeMap
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	static inline spk::OpenGL::ShaderStorageBufferObject* _nodeMapSSBO = nullptr;
	std::vector<Node> _content;

public:
	NodeMap()
	{
		if (BufferObjectCollection::instance()->isAllocated("nodeConstants") == false)
		{
			_nodeMapSSBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 4, 4, 20, 4);

			_nodeMapSSBO->addElement("nbNodes", 0, 4);

			_nodeMapSSBO->operator[]("nbNodes") = 0;

			_nodeMapSSBO->validate();
		}
		else
		{
			_nodeMapSSBO = BufferObjectCollection::instance()->SSBO("nodeConstants");
		}
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
		
		_nodeMapSSBO->operator[]("nbNodes") = int(_content.size());
		_nodeMapSSBO->resizeDynamicArray(_content.size());
		_nodeMapSSBO->dynamicArray() = _content;
		_nodeMapSSBO->validate();
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
	static inline spk::OpenGL::UniformBufferObject* _cameraUBO = nullptr;
	static inline spk::OpenGL::UniformBufferObject* _chunkTextureInfo = nullptr;
	static inline spk::OpenGL::ShaderStorageBufferObject* _nodeMapSSBO = nullptr;
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

		const char *vertexShaderSrc = R"(#version 450 core

			layout(location = 0) in vec2 inDelta;

			layout (std140, binding = 0) uniform Transform_Type
			{
				mat4 model;
			} transform;

			layout(std140, binding = 1) uniform Camera_Type
			{
				mat4 view;
				mat4 projection;
			} camera;

			layout(std430, binding = 2) buffer CellList_Type
			{
				ivec3 cellList[];
			};

			layout(std430, binding = 3) buffer Chunk_Type
			{
				int content[16][16][5];
			} chunk;

			struct Node
			{
				vec2 animationStartPos;
				int frameDuration;
				int animationLength;
				int animationStep;
			};

			layout(std430, binding = 4) buffer NodeConstants_Type
			{
				int nbNodes;
				Node nodes[];
			} nodeConstants;

			layout(std140, binding = 5) uniform ChunkTextureInfo_Type
			{
				vec2 unit;
			} chunkTextureInfo;

			layout(location = 0) out vec2 outUV;

			void main()
			{
				ivec3 cellPosition = cellList[gl_InstanceID];
				ivec3 chunkRelPosition = ivec3(inDelta, 0) + cellPosition;
				vec4 worldPosition = transform.model * vec4(chunkRelPosition, 1);
				vec4 viewPosition = camera.view * worldPosition;
				gl_Position = camera.projection * viewPosition;

				int nodeIndex = chunk.content[cellPosition.x][cellPosition.y][cellPosition.z];

				if (nodeIndex < 0 || nodeIndex >= nodeConstants.nbNodes)
				{
					outUV = vec2(-1, -1);
					return;
				}

				Node node = nodeConstants.nodes[nodeIndex];

				vec2 spritePos = (node.animationStartPos + vec2(node.animationStep * node.frameDuration, 0) + inDelta);
				outUV = spritePos * chunkTextureInfo.unit;
			})";

		const char *fragmentShaderSrc = R"(#version 450

			layout(location = 0) in vec2 inUV;

			layout(location = 0) out vec4 outputColor;

			uniform sampler2D spriteSheet;

            void main()
            {
				if (inUV.x < 0 || inUV.y < 0)
				{
					discard;
				}
				else
				{
					outputColor = texture(spriteSheet, inUV);
				}
			})";

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

		if (_cameraUBO == nullptr)
		{
			if (BufferObjectCollection::instance()->isAllocated("camera") == false)
			{
				_cameraUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 1, 128);
			
				_cameraUBO->addElement("view", 0, 64);
				_cameraUBO->addElement("projection", 64, 64);

				(*_cameraUBO)["view"] = spk::Matrix4x4::identity();
				(*_cameraUBO)["projection"] = spk::Matrix4x4::identity();

				_cameraUBO->validate();
			}
			else
			{
				_cameraUBO = BufferObjectCollection::instance()->UBO("camera");
			}
		}

		if (_nodeMapSSBO == nullptr)
		{
			if (BufferObjectCollection::instance()->isAllocated("nodeConstants") == false)
			{
				_nodeMapSSBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 4, 4, 20, 4);

				_nodeMapSSBO->addElement("nbNodes", 0, 4);

				_nodeMapSSBO->operator[]("nbNodes") = 0;

				_nodeMapSSBO->validate();
			}
			else
			{
				_nodeMapSSBO = BufferObjectCollection::instance()->SSBO("nodeConstants");
			}
		}

		if (_chunkTextureInfo == nullptr)
		{
			if (BufferObjectCollection::instance()->isAllocated("chunkTextureInfo") == false)
			{
				_chunkTextureInfo = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("chunkTextureInfo", "ChunkTextureInfo_Type", 5, 8);

				_chunkTextureInfo->addElement("unit", 0, 8);

				_chunkTextureInfo->operator[]("unit") = spk::Vector2(1, 1);

				_chunkTextureInfo->validate();
			}
			else
			{
				_chunkTextureInfo = BufferObjectCollection::instance()->UBO("chunkTextureInfo");
			}
		}

		_chunkSSBO = spk::OpenGL::ShaderStorageBufferObject("Chunk_Type", 3, 5120, 0, 0, 0);

		_chunkSSBO.addElement("content", 0, 4, 1280, 0);

		_chunkSSBO["content"] = std::vector<int>(16 * 16 * 5, -1);

		_chunkSSBO.validate();

		_spriteSheetSampler = spk::OpenGL::SamplerObject("spriteSheet", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
	}

public:
	ChunkRenderer()
	{
		_initProgram();
		_initBuffers();
	}

	void updateSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_spriteSheetSampler.bind(p_spriteSheet);
		(*_chunkTextureInfo)["unit"] = p_spriteSheet->unit();
		_chunkTextureInfo->validate();
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

		_spriteSheetSampler.activate();
		_chunkSSBO.activate();
		_chunkTextureInfo->activate();
		_nodeMapSSBO->activate();
		_cellListSSBO.activate();
		_cameraUBO->activate();
		_transformUBO.activate();
		_bufferSet.activate();
		
		_program->render(_bufferSet.indexes().nbIndexes(), _cellListSSBO.dynamicArray().nbElement());

		_bufferSet.deactivate();
		_transformUBO.deactivate();
		_cameraUBO->deactivate();
		_cellListSSBO.deactivate();
		_nodeMapSSBO->deactivate();
		_chunkTextureInfo->deactivate();
		_chunkSSBO.deactivate();
		_spriteSheetSampler.deactivate();

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
		_renderer.updateChunk(_chunk);
		_renderer.updateChunkData(_chunk);
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
	static inline spk::OpenGL::UniformBufferObject* _cameraUBO = nullptr;

	spk::Camera _camera;

	spk::Entity::Contract _onEditionContract;

public:
	CameraComponent(const std::wstring& p_name) :
		spk::Component(p_name)
	{
		if (_cameraUBO == nullptr)
		{
			if (BufferObjectCollection::instance()->isAllocated("camera") == false)
			{
				_cameraUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 1, 128);
			
				_cameraUBO->addElement("view", 0, 64);
				_cameraUBO->addElement("projection", 64, 64);

				(*_cameraUBO)["view"] = spk::Matrix4x4::identity();
				(*_cameraUBO)["projection"] = spk::Matrix4x4::identity();

				_cameraUBO->validate();
			}
			else
			{
				_cameraUBO = BufferObjectCollection::instance()->UBO("camera");
			}
		}

		setOrthographic(-20, 20, -20, 20);
	}

	void start() override
	{
		_onEditionContract = owner()->transform().addOnEditionCallback([&](){
			(*_cameraUBO)["view"] = owner()->transform().model();
		});
		
		_onEditionContract.trigger();
	}

	void setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane = 0.1f, float p_farPlane = 1000.0f)
	{
		_camera.setPerspective(p_fovDegrees, p_aspectRatio, p_nearPlane, p_farPlane);
		(*_cameraUBO)["projection"] = _camera.projectionMatrix();
	}
        
	void setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane= 0.1f, float p_farPlane = 1000.0f)
	{
		_camera.setOrthographic(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
		(*_cameraUBO)["projection"] = _camera.projectionMatrix();
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
		.frameDuration = 100,
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

	BakableChunk chunk;

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
					chunk.setContent(x, y, z, 0);
				}
				else if (z == 1 && (
					x == 1 || x == Chunk::Size - 2 || 
					y == 1 || y == Chunk::Size - 2))
				{
					chunk.setContent(x, y, z, 1);
				}
				else if (z == 2 && (
					x == 2 || x == Chunk::Size - 3 || 
					y == 2 || y == Chunk::Size - 3))
				{
					chunk.setContent(x, y, z, 2);
				}
				else if (z == 3 && (
					x == 3 || x == Chunk::Size - 4 || 
					y == 3 || y == Chunk::Size - 4))
				{
					chunk.setContent(x, y, z, 3);
				}
			}
		}
	}

	chunk.invalidate();

	spk::GameEngine engine;

	spk::Entity player = spk::Entity(L"Player");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, -10));

	CameraComponent& cameraComp = camera.addComponent<CameraComponent>(L"Main camera");

	spk::Entity chunkObject = spk::Entity(L"Chunk");
	chunkObject.transform().place(spk::Vector3(0, 0, 0));
	ChunkComponent& chunkComp = chunkObject.addComponent<ChunkComponent>();
	chunkComp.setChunk(&chunk);
	chunkComp.setSpriteSheet(&chunkSpriteSheet);

	engine.addEntity(&player);
	engine.addEntity(&chunkObject);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}