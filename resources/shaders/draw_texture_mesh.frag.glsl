#version 460 core

uniform sampler2D uTexture;

layout(location = 0) in vec2 vertexUV;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(uTexture, vertexUV);
	if (outColor.a <= 0.0)
	{
		discard;
	}
}
