#version 460 core

uniform sampler2D uAlbedo;

layout(location = 0) in vec2 vertexUV;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(texture(uAlbedo, vertexUV).rgb, 1.0);
}
