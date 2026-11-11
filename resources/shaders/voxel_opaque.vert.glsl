#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inMaterialIndex;

layout(std430) readonly buffer Instances
{
	mat4 models[];
};

uniform mat4 uViewProjection;

layout(location = 0) out vec2 vertexUV;
layout(location = 1) flat out uint materialIndex;
layout(location = 2) out vec3 worldNormal;

void main()
{
	mat4 model = models[gl_InstanceID];
	gl_Position = uViewProjection * model * vec4(inPosition, 1.0);
	vertexUV = inUV;
	materialIndex = inMaterialIndex;
	worldNormal = normalize(mat3(model) * inNormal);
}
