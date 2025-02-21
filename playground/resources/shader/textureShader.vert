#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main()
{
	gl_position = vec4(inPosition, 1);
	outUV = inUV;
}