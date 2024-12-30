#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outputColor;

uniform sampler2D spriteSheet;

void main()
{
	if (inUV.x < 0 || inUV.y < 0)
	{
		discard;
	}
	else
	{
		outputColor = texture(spriteSheet, inUV);
	}
}