#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

uniform mat4 uProjection;
uniform mat4 uViewRotation;

layout(location = 0) out vec2 vertexUV;

void main()
{
	vec4 projected = uProjection * uViewRotation * vec4(inPosition, 1.0);
	gl_Position = projected.xyww;
	vertexUV = inUV;
}
