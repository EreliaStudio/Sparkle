#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outputColor;

uniform sampler2D diffuseTexture;

void main()
{
	outputColor = texture(diffuseTexture, inUV);
}