#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inMaterialIndex;

uniform mat4 uViewProjection;
uniform mat4 uModel;

layout(location = 0) out vec2 vertexUV;
layout(location = 1) flat out uint materialIndex;
layout(location = 2) out vec3 worldNormal;

void main()
{
	gl_Position = uViewProjection * uModel * vec4(inPosition, 1.0);
	vertexUV = inUV;
	materialIndex = inMaterialIndex;
	worldNormal = normalize(mat3(uModel) * inNormal);
}
