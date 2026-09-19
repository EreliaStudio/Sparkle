#version 460 core

uniform sampler2D uAtlas;

layout(std140) uniform FontRenderData
{
	vec4 uGlyphColor;
	vec4 uOutlineColor;
	float uOutlineThickness;
};

layout(location = 0) in vec2 vertexUV;
layout(location = 0) out vec4 outColor;

void main()
{
	const float fillEdge = 0.5;
	const float smoothing = 0.05;
	float sdf = texture(uAtlas, vertexUV).r;
	float outlineEdge = fillEdge - uOutlineThickness;
	float fillAlpha = smoothstep(fillEdge - smoothing, fillEdge + smoothing, sdf);
	float outlineAlpha = uOutlineThickness > 0
		? smoothstep(outlineEdge - smoothing, outlineEdge + smoothing, sdf)
		: 0;
	vec4 outline = vec4(
		uOutlineColor.rgb * uOutlineColor.a * outlineAlpha,
		uOutlineColor.a * outlineAlpha);
	vec4 fill = vec4(
		uGlyphColor.rgb * uGlyphColor.a * fillAlpha,
		uGlyphColor.a * fillAlpha);
	vec4 premultiplied = fill + outline * (1 - fill.a);
	vec3 color = premultiplied.a > 0
		? premultiplied.rgb / premultiplied.a
		: vec3(0);
	outColor = vec4(color, premultiplied.a);
}
