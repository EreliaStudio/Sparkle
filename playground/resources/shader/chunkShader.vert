#version 450 core

layout(location = 0) in vec2 inDelta;

layout (std140, binding = 0) uniform Transform_Type
{
	mat4 model;
} transform;

layout(std140, binding = 1) uniform Camera_Type
{
	mat4 view;
	mat4 projection;
} camera;

layout(std430, binding = 2) buffer CellList_Type
{
	ivec3 cellList[];
};

layout(std430, binding = 3) buffer Chunk_Type
{
	int content[16][16][5];
} chunk;

struct Node
{
	vec2 animationStartPos;
	int frameDuration;
	int animationLength;
	int animationStep;
};

layout(std430, binding = 4) buffer NodeConstants_Type
{
	int nbNodes;
	Node nodes[];
} nodeConstants;

layout(std140, binding = 5) uniform ChunkTextureInfo_Type
{
	vec2 unit;
} chunkTextureInfo;

layout(std140, binding = 6) uniform SystemInfo_Type
{
	int time;
} systemInfo;

layout(location = 0) out vec2 outUV;

void main()
{
	ivec3 cellPosition = cellList[gl_InstanceID];
	ivec3 chunkRelPosition = ivec3(inDelta, 0) + cellPosition;
	vec4 worldPosition = transform.model * vec4(chunkRelPosition, 1);
	vec4 viewPosition = camera.view * worldPosition;
	gl_Position = camera.projection * viewPosition;

	int nodeIndex = chunk.content[cellPosition.x][cellPosition.y][cellPosition.z];

	if (nodeIndex < 0 || nodeIndex >= nodeConstants.nbNodes)
	{
		outUV = vec2(-1, -1);
		return;
	}

	Node node = nodeConstants.nodes[nodeIndex];

	int frameIndex = 0;
	
	if (node.animationLength != 0)
	{
		frameIndex = (systemInfo.time / node.frameDuration) % node.animationLength;
	}
	
	vec2 spritePos = (node.animationStartPos + vec2(node.animationStep * frameIndex, 0) + inDelta);
	outUV = spritePos * chunkTextureInfo.unit;
}