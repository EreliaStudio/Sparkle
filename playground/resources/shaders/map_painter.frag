#version 450

layout(location = 0) in vec2 fragUVs;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D uTileset;

void main()
{
    if (fragUVs.x < 0.0 || fragUVs.y < 0.0)
	{
        discard;
    }

    vec4 texel = texture(uTileset, fragUVs);

    if (texel.a <= 0.0)
	{
        discard;
    }

    outColor = texel;
}