#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inMaterialIndex;

layout(std430) readonly buffer Instances
{
	mat4 models[];
};

layout(std140, binding = 1) uniform CameraData
{
	mat4 uProjection;
	mat4 uView;
	mat4 uInverseView;
	mat4 uViewProjection;
};

layout(location = 0) out vec2 vertexUV;
layout(location = 1) flat out uint materialIndex;
layout(location = 2) out vec3 worldNormal;
layout(location = 3) out vec3 worldPosition;

void main()
{
	mat4 model = models[gl_InstanceID];
	vec4 position = model * vec4(inPosition, 1.0);
	gl_Position = uViewProjection * position;
	worldPosition = position.xyz;
	vertexUV = inUV;
	materialIndex = inMaterialIndex;
	worldNormal = normalize((model * vec4(inNormal, 0.0)).xyz);
}
