#version 460 core

uniform sampler2D uTexture;

layout(std140) uniform NineSliceRenderData
{
	ivec4 uGeometry;
	uvec4 uCorner;
};

layout(location = 0) noperspective in vec2 vertexPosition;
layout(location = 0) out vec4 outColor;

void resolveAxis(int coordinate, int size, int corner, out int cell, out int offset, out int span)
{
	if (coordinate < corner)
	{
		cell = 0;
		offset = coordinate;
		span = corner;
		return;
	}
	if (coordinate >= size - corner)
	{
		cell = 2;
		offset = coordinate - (size - corner);
		span = corner;
		return;
	}
	cell = 1;
	offset = coordinate - corner;
	span = size - 2 * corner;
}

int sourceOffset(int offset, int sourceSpan, int destinationSpan)
{
	if (destinationSpan <= 0)
	{
		return 0;
	}
	int numerator = (2 * offset + 1) * sourceSpan;
	int denominator = 2 * destinationSpan;
	return min(numerator / denominator, sourceSpan - 1);
}

void main()
{
	ivec2 local = ivec2(floor(vertexPosition)) - uGeometry.xy;
	ivec2 destinationSize = uGeometry.zw;
	ivec2 corner = ivec2(uCorner.xy);

	int cellX;
	int offsetX;
	int spanX;
	int cellY;
	int offsetY;
	int spanY;
	resolveAxis(local.x, destinationSize.x, corner.x, cellX, offsetX, spanX);
	resolveAxis(local.y, destinationSize.y, corner.y, cellY, offsetY, spanY);

	ivec2 sourceCellSize = textureSize(uTexture, 0) / 3;
	ivec2 source = ivec2(cellX, cellY) * sourceCellSize;
	source += ivec2(
		sourceOffset(offsetX, sourceCellSize.x, spanX),
		sourceOffset(offsetY, sourceCellSize.y, spanY));

	outColor = texelFetch(uTexture, source, 0);
	if (outColor.a <= 0.0)
	{
		discard;
	}
}
