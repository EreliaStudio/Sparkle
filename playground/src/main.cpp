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
	BakableChunk* _chunk;
	spk::SpriteSheet* _spriteSheet = nullptr;

	static inline spk::OpenGL::Program* _program = nullptr;
	static inline spk::OpenGL::UniformBufferObject* _cameraUBO = nullptr;
	static inline spk::OpenGL::UniformBufferObject* _timeUBO = nullptr;
	static inline spk::OpenGL::ShaderStorageBufferObject* _nodeMapSSBO = nullptr;
	static inline spk::OpenGL::SamplerObject* _samplerObject;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::UniformBufferObject _transformUBO;
	spk::OpenGL::UniformBufferObject _chunkDataUBO;

	void _initProgram()
	{
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
				ivec2 data[];
			} instanceData;

			layout(location = 0) out vec2 fragmentUVs;

			void main()
			{
				vec2 cellPosition = instanceData[gl_InstanceID];
				vec4 worldPos = transform.model * vec4(cellPosition + inDelta, 0.0, 1.0);
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
		if (_cameraUBO == nullptr)
		{
			bool needInstanciation = BufferObjectCollection::instance()->isAllocated("camera") == false;

			_cameraUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("camera", "Camera_Type", 0, 128);
			
			if (needInstanciation == true)
			{
				_cameraUBO->addElement("projection", 0, 64);
				_cameraUBO->addElement("view", 64, 64);

				_cameraUBO->operator[]("projection") = spk::Matrix4x4();
				_cameraUBO->operator[]("view") = spk::Matrix4x4();

				_cameraUBO->validate();
			}
		}
		
		if (_timeUBO == nullptr)
		{
			bool needInstanciation = BufferObjectCollection::instance()->isAllocated("timeConstants") == false;

			_timeUBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::UniformBufferObject>("timeConstants", "TimeConstants_Type", 0, 4);
			
			if (needInstanciation == true)
			{
				_timeUBO->addElement("time", 0, 4);

				_timeUBO->operator[]("time") = 0;

				_timeUBO->validate();
			}
		}
		
		if (_nodeMapSSBO == nullptr)
		{
			bool needInstanciation = BufferObjectCollection::instance()->isAllocated("nodeConstants") == false;

			_nodeMapSSBO = BufferObjectCollection::instance()->allocate<spk::OpenGL::ShaderStorageBufferObject>("nodeConstants", "NodeConstants_Type", 0, 4, 20);
			
			if (needInstanciation == true)
			{
				_nodeMapSSBO->addElement("nbNodes", 0, 4);
			}
		}
	
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2Int} // inCellIndex
		});

		if (_samplerObject == nullptr)
		{
			_samplerObject = new spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
		}

		_transformUBO = spk::OpenGL::UniformBufferObject("transform", 2, 64);
		_transformUBO.addElement("model", 0, 64);

		_chunkDataUBO = spk::OpenGL::UniformBufferObject("chunkData", 3, 5120);	
		_chunkDataUBO.addElement("chunkData", 0, 4, 16 * 16 * 5);
	}

	void _bake()
	{
		_chunkDataUBO["chunkData"] = *_chunk;
		_chunkDataUBO.validate();
	}

public:
	ChunkRenderer() :
		_chunk(nullptr)
	{
		if (_program == nullptr)
		{
			_initProgram();
		}
		_initBuffers();
	}

	void setChunk(BakableChunk* p_chunk)
	{
		_chunk = p_chunk;
	}

	void setSpriteSheet(spk::SpriteSheet* p_spriteSheet)
	{
		_spriteSheet = p_spriteSheet;
		
		_samplerObject->bind(_spriteSheet);
	}

	void render()
	{
		if (_chunk == nullptr)
		{
			return;
		}

		if (_chunk->needBake() == true)
		{
			_bake();
			_chunk->validate();
		}

		_program->activate();

		_cameraUBO->activate();
		_timeUBO->activate();
		_nodeMapSSBO->activate();
		_samplerObject->activate();
		_transformUBO.activate();
		_chunkDataUBO.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_chunkDataUBO.deactivate();
		_transformUBO.deactivate();
		_samplerObject->deactivate();
		_nodeMapSSBO->deactivate();
		_timeUBO->deactivate();
		_cameraUBO->deactivate();
		
		_program->deactivate();
	}
};

class Tilemap
{

};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {840, 680}});

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.activate();

	return (app.run());
}