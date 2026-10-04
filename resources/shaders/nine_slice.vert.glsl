#version 460 core

layout(location = 0) in vec2 inPosition;
layout(location = 1) in float inDepth;

layout(std140) uniform ViewportData
{
	mat4 uProjection;
};

layout(location = 0) noperspective out vec2 vertexPosition;

void main()
{
	gl_Position = uProjection * vec4(inPosition, inDepth, 1.0);
	vertexPosition = inPosition;
}
