#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 1) uniform CameraData
{
	mat4 uProjection;
	mat4 uView;
	mat4 uInverseView;
	mat4 uViewProjection;
};

layout(location = 0) out vec2 vertexUV;

void main()
{
	vec4 projected = uProjection * mat4(mat3(uView)) * vec4(inPosition, 1.0);
	gl_Position = projected.xyww;
	vertexUV = inUV;
}
