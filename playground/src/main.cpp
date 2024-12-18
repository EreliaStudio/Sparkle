#include "playground.hpp"

struct Node
{

};

template<size_t Size, size_t Layer>
class Chunk
{
private:
	short _content[Size][Size][Layer];

public:
	Chunk()
	{
		std::memset(&_content, -1, sizeof(_content));
	}

	void setContent(const spk::Vector3Int& p_relPosition, short p_value)
	{
		_content[p_relPosition.x][p_relPosition.y][p_relPosition.z] = p_value;
	}

	void setContent(const spk::Vector2Int& p_relPosition, const int& p_layer, short p_value)
	{
		_content[p_relPosition.x][p_relPosition.y][p_layer] = p_value;
	}

	void setContent(const int& p_x, const int& p_y, const int& p_z, short p_value)
	{
		_content[p_x][p_y][p_z] = p_value;
	}

	void content(const spk::Vector3Int& p_relPosition)
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_relPosition.z]);
	}

	void content(const spk::Vector2Int& p_relPosition, const int& p_layer)
	{
		return (_content[p_relPosition.x][p_relPosition.y][p_layer]);
	}

	void content(const int& p_x, const int& p_y, const int& p_z)
	{
		return (_content[p_x][p_y][p_z]);
	}
};


template<size_t Size, size_t Layer>
class BakableChunk : public Chunk<Size, Layer>
{
private:
	bool _needBake;

	spk::OpenGL::Program _program;
	spk::OpenGL::BufferSet _bufferSet;
	spk::OpenGL::SamplerObject _samplerObject;

	void _initProgram()
	{
		const char *vertexShaderSrc = R"(#version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec2 inUV;

			layout (std140, binding = 0) uniform Camera_Type
			{
				mat4 projection;
				mat4 view;
			} camera;

            layout(location = 0) out vec2 fragUV;

            void main()
            {
                gl_Position = vec4(inPosition, inLayer, 1.0);
                fragUV = inUV;
            }
            )";

		const char *fragmentShaderSrc = R"(#version 450
            layout(location = 0) in vec2 fragUV;
            layout(location = 0) out vec4 outputColor;

            uniform sampler2D diffuseTexture;

            void main()
            {
                outputColor = texture(diffuseTexture, fragUV);
            }
            )";

		_program = spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
	}

	void _initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3}, // position
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}	// uv
		});

		_samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);

		_cameraUBO = spk::OpenGL::UniformBufferObject("Camera_Type", 0, 128);
		_cameraUBO.addElement("projection", 0, sizeof(spk::Matrix4));
		_cameraUBO.addElement("view", 64, sizeof(spk::Matrix4));
	}

public:
	BakableChunk() :
		Chunk()
	{

	}

	void validate()
	{
		_needBake = true;
	}

	void bake()
	{


		_needBake = false;
	}
};

class ChunkRenderer
{
private:
	BakableChunk* _chunk;

public:
	ChunkRenderer() :
		_chunk(nullptr)
	{

	}

	void setChunk(BakableChunk* p_chunk)
	{
		_chunk = p_chunk;
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