#include "playground.hpp"

#include "structure/graphics/opengl/spk_pipeline.hpp"

// int main()
// {
// 	spk::GraphicalApplication app = spk::GraphicalApplication();

// 	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {800, 800});
	
// 	return (app.run());
// }

int main()
{
	std::string vertexShaderCode = R"(
#version 450 core

layout (location = 0) in vec2 modelVertex;

layout (location = 0) out vec4 fragmentColor;

struct ModelData
{
	vec4 color;
	vec2 position;
	vec2 size;
	float rotation;
};

layout(std430, type = Attribute) uniform ModelDatasSSBO
{
	ModelData modelData[];
} modelDatas;

layout (std140, type = Constant) uniform CameraData
{
	mat4 view;
	mat4 projection;
} cameraData;

int main()
{
	// Set the fragment color based on the model data
	fragmentColor = modelDatas.modelData[0].color;

	// Calculate the vertex position in world space
	vec4 worldPosition = cameraData.view * vec4(modelVertex, 0.0, 1.0);
	gl_Position = cameraData.projection * worldPosition;
}
)";

	std::string fragmentShaderCode = R"(
#version 450 core
layout (location = 0) in vec4 fragmentColor;
layout (location = 0) out vec4 color;
void main()
{
	// Set the output color
	color = fragmentColor;
}
)";


	spk::Pipeline pipeline(vertexShaderCode, fragmentShaderCode);


	return (0);
}