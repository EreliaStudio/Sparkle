#version 460 core

layout(location = 0) in vec2 inPosition;
layout(location = 1) in float inDepth;
layout(location = 2) in vec2 inUV;

layout(std140) uniform ViewportData
{
	mat4 uProjection;
};

layout(location = 0) out vec2 vertexUV;

void main()
{
	gl_Position = uProjection * vec4(inPosition, inDepth, 1.0);
	vertexUV = inUV;
}
