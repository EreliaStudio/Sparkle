#include "playground.hpp"

struct Node
{
	spk::Vector2 animationStartPos;
	int frameDuration;
	int animationLength;
	int animationStep;
};

class NodeMap
{
private:
	std::vector<Node> _content;

public:
	NodeMap()
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

	const Node& operator[](const int& p_id) const
	{
		if (p_id == -1)
		{
			throw std::runtime_error("Invalid node ID");
		}
		return (_content[p_id]);
	}
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

	int content(const spk::Vector3Int& p_relPosition)
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_relPosition.z]);
	}

	int content(const spk::Vector2Int& p_relPosition, const int& p_layer)
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_layer]);
	}

	int content(const int& p_x, const int& p_y, const int& p_z)
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

class BakableChunk : public Chunk
{
private:
	bool _needBake;

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
	spk::SpriteSheet* _spriteSheet = nullptr;

	static inline spk::OpenGL::Program* _program = nullptr;
	static inline spk::OpenGL::UniformBufferObject* _cameraUBO = nullptr;
	static inline spk::OpenGL::UniformBufferObject* _timeUBO = nullptr;
	static inline spk::OpenGL::ShaderStorageBufferObject* _nodeMapSSBO = nullptr;
	static inline spk::OpenGL::SamplerObject* _samplerObject;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::UniformBufferObject _transformUBO;
	spk::OpenGL::UniformBufferObject _chunkDataUBO;
	spk::OpenGL::ShaderStorageBufferObject _instanceDataSSBO;

	void _initProgram()
	{
		if (_program != nullptr)
		{
			return;
		}

		const char *vertexShaderSrc = R"(#version 450 core

			layout(location = 0) in vec2 inDelta;

			layout(std140, binding = 0) uniform Camera_Type
			{
				mat4 projection;
				mat4 view;
			} camera;

			layout(std140, binding = 1) uniform TimeConstants_Type
			{
				int time; // Express in milliseconds
			} timeConstants;

			layout(std140, binding = 2) uniform Transform_Type
			{
				mat4 model;
			} transform;

			layout(std140, binding = 3) uniform ChunkData_Type
			{
				int chunkData[16][16][5];
			} chunkData;

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

			layout(std430, binding = 5) buffer InstanceData_Type
			{
				ivec3 data[];
			} instanceData;

			layout(location = 0) out vec2 fragmentUVs;

			void main()
			{
				vec3 cellPosition = instanceData.data[gl_InstanceID];
				vec4 worldPos = transform.model * vec4(vec3(inDelta, 0.0) + cellPosition, 1.0);
				gl_Position = camera.projection * camera.view * worldPos;

				int nodeIndex = chunkData[cellPosition.x][cellPosition.y][cellPosition.z];
				if (nodeIndex < 0 || nodeIndex >= nodeConstants.nbNodes)
				{
					fragmentUVs = vec2(-1.0, -1.0);
					return;
				}
				Node node = nodeConstants.nodes[nodeIndex];

				int frameIndex = (timeConstants.time / node.frameDuration) % node.animationLength;

				vec2 baseUV = node.animationStartPos + inDelta;
				vec2 offsetUV = vec2(frameIndex * node.animationStep, 0.0);

				fragmentUVs = baseUV + offsetUV;
			})";

		const char *fragmentShaderSrc = R"(#version 450
            layout(location = 0) in vec2 fragUV;
			layout(location = 0) out vec4 outputColor;

            uniform sampler2D diffuseTexture;

            void main()
            {
				outputColor = texture(diffuseTexture, fragUV);
			})";

		_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
	}

	void _initBuffers()
	{
		if (BufferObjectCollection::instance()->isAllocated("camera") == false)
		{
			_cameraUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 0, 128);
			_cameraUBO->addElement("projection", 0, 64);
			_cameraUBO->addElement("view", 64, 64);

			_cameraUBO->operator[]("projection") = spk::Matrix4x4();
			_cameraUBO->operator[]("view") = spk::Matrix4x4();

			_cameraUBO->validate();
		}
		else
		{
			_cameraUBO = BufferObjectCollection::instance()->UBO("camera");
		}
		
		if (BufferObjectCollection::instance()->isAllocated("timeConstants") == false)
		{
			_timeUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("timeConstants", "TimeConstants_Type", 0, 4);
			
			_timeUBO->addElement("time", 0, 4);

			_timeUBO->operator[]("time") = 0;

			_timeUBO->validate();
		}
		else
		{
			_timeUBO = BufferObjectCollection::instance()->UBO("timeConstants");
		}
		
		if (BufferObjectCollection::instance()->isAllocated("nodeConstants") == false)
		{
			_nodeMapSSBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 0, 20);

			_nodeMapSSBO->validate();
		}
		else
		{
			_nodeMapSSBO = BufferObjectCollection::instance()->SSBO("nodeConstants");
		}
	
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2Int} // inCellIndex
		});

		_bufferSet.layout() << spk::Vector2Int(0, 0);
		_bufferSet.layout() << spk::Vector2Int(0, 1);
		_bufferSet.layout() << spk::Vector2Int(1, 0);
		_bufferSet.layout() << spk::Vector2Int(1, 1);

		_bufferSet.indexes() << 0 << 1 << 2 << 2 << 1 << 3;

		if (_samplerObject == nullptr)
		{
			_samplerObject = new spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
		}

		_transformUBO = spk::OpenGL::UniformBufferObject("transform", 2, 64);
		_transformUBO.addElement("model", 0, 64);

		_chunkDataUBO = spk::OpenGL::UniformBufferObject("chunkData", 3, 5120);	
		_chunkDataUBO.addElement("chunkData", 0, 4, 16 * 16 * 5);

		_instanceDataSSBO = spk::OpenGL::ShaderStorageBufferObject("instanceData", 6, 0, 12);
	}

public:
	ChunkRenderer()
	{
		_initProgram();
		_initBuffers();
	}

	void setSpriteSheet(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet)
	{
		_samplerObject->bind(p_spriteSheet);
	}

	void bakeTransform(const spk::SafePointer<spk::Entity>& entity)
	{
		_transformUBO["model"] = entity->transform().model();
	}

	void bakeChunk(spk::SafePointer<BakableChunk> p_chunk)
	{
		_chunkDataUBO["chunkData"] = *p_chunk;
		_chunkDataUBO.validate();

		std::vector<spk::Vector3Int> instanceData;

		for (size_t x = 0; x < Chunk::Size; x++)
		{
			for (size_t y = 0; y < Chunk::Size; y++)
			{
				for (size_t z = 0; z < Chunk::Layer; z++)
				{
					int nodeIndex = p_chunk->content(x, y, z);
					if (nodeIndex != -1)
					{
						instanceData.push_back(spk::Vector3Int(x, y, z));
					}
				}	
			}
		}

		_instanceDataSSBO.dynamicArray() = instanceData;
		_instanceDataSSBO.validate();
	}

	void render()
	{
		_program->activate();

		_cameraUBO->activate();
		_timeUBO->activate();
		_nodeMapSSBO->activate();
		_samplerObject->activate();
		_transformUBO.activate();
		_chunkDataUBO.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), _instanceDataSSBO.dynamicArray().nbElement());

		_chunkDataUBO.deactivate();
		_transformUBO.deactivate();
		_samplerObject->deactivate();
		_nodeMapSSBO->deactivate();
		_timeUBO->deactivate();
		_cameraUBO->deactivate();
		
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

	void setSpriteSheet(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet)
	{
		_renderer.setSpriteSheet(p_spriteSheet);
	}

	void start() override
	{
		_onEditionContract = owner()->transform().addOnEditionCallback([&](){
			_renderer.bakeTransform(owner());
		});
		_onEditionContract.trigger();
	}

	void onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (_chunk->needBake() == true)
		{
			_renderer.bakeChunk(_chunk);
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
	DEBUG_LINE();
		if (_cameraUBO == nullptr)
		{
	DEBUG_LINE();
			if (BufferObjectCollection::instance()->isAllocated("camera") == false)
			{
	DEBUG_LINE();
				_cameraUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 0, 128);
				_cameraUBO->addElement("projection", 0, 64);
				_cameraUBO->addElement("view", 64, 64);
	DEBUG_LINE();

				_cameraUBO->operator[]("projection") = spk::Matrix4x4();
				_cameraUBO->operator[]("view") = spk::Matrix4x4();
	DEBUG_LINE();

				_cameraUBO->validate();
	DEBUG_LINE();
			}
			else
			{
	DEBUG_LINE();
				_cameraUBO = BufferObjectCollection::instance()->UBO("camera");
	DEBUG_LINE();
			}
	DEBUG_LINE();
		}

	DEBUG_LINE();
		
	DEBUG_LINE();

		setOrthographic(-10, 10, -10, 10);
	DEBUG_LINE();
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
	DEBUG_LINE();
	spk::GraphicalApplication app = spk::GraphicalApplication();

	DEBUG_LINE();
	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {840, 680}});

	DEBUG_LINE();
	BakableChunk chunk;

	DEBUG_LINE();
	spk::GameEngine engine;

	DEBUG_LINE();
	spk::Entity player = spk::Entity(L"Player");

	DEBUG_LINE();
	spk::Entity camera = spk::Entity(L"Camera", &player);
	DEBUG_LINE();
	camera.transform().place(spk::Vector3(0, 0, -10));

	DEBUG_LINE();
	CameraComponent& cameraComp = camera.addComponent<CameraComponent>(L"Main camera");

	DEBUG_LINE();
	spk::Entity chunkObject = spk::Entity(L"Chunk");
	DEBUG_LINE();
	ChunkComponent& chunkComp = chunkObject.addComponent<ChunkComponent>();
	DEBUG_LINE();
	chunkComp.setChunk(&chunk);
	DEBUG_LINE();

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.activate();
	DEBUG_LINE();

	return (app.run());
}