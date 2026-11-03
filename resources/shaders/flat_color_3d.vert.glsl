#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

uniform mat4 uViewProjection;

layout(location = 0) out vec3 vertexColor;

void main()
{
	gl_Position = uViewProjection * vec4(inPosition, 1.0);
	vertexColor = inColor;
}
