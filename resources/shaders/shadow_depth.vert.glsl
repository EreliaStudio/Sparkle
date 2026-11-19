#version 460 core

layout(location = 0) in vec3 inPosition;

layout(std140, binding = 4) uniform ShadowCamera
{
	mat4 uShadowViewProjection;
};

layout(std140, binding = 5) uniform ShadowModel
{
	mat4 uShadowModel;
};

void main()
{
	gl_Position = uShadowViewProjection * uShadowModel * vec4(inPosition, 1.0);
}
