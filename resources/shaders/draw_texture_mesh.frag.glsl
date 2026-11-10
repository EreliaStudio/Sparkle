#version 460 core

uniform sampler2D uTexture;
uniform bool uEncodeSRGB;

layout(location = 0) in vec2 vertexUV;
layout(location = 0) out vec4 outColor;

vec3 encodeSRGB(vec3 linear)
{
	vec3 low = linear * 12.92;
	vec3 high = 1.055 * pow(max(linear, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
	return mix(low, high, greaterThan(linear, vec3(0.0031308)));
}

void main()
{
	outColor = texture(uTexture, vertexUV);
	if (uEncodeSRGB)
	{
		outColor.rgb = encodeSRGB(outColor.rgb);
	}
	if (outColor.a <= 0.0)
	{
		discard;
	}
}
