#version 450

layout(location = 0) in vec2 modelPosition; // Expressed from 0 to 1, describing a square or 1 world unit of size

layout(std140, binding = 0) uniform CameraUBO {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} cameraUBO;

layout(std140, binding = 1) uniform TimeUBO {
    uint epoch;
} timeUBO;

const int GRID_W = 16;
const int GRID_H = 16;
const int GRID_LAYER = 5;

layout(std430, binding = 2) buffer ChunkDataUBO {
    ivec2 position;
    int data[GRID_W * GRID_H * GRID_LAYER];
    int activeCells[];
} chunkDataUBO;

struct Tile {
    ivec2 coordinates;
    int nbFrames;
    ivec2 offsetBetweenFrames;
    uint animationDuration;
};

layout(std430, binding = 3) buffer TilesetSSBO {
    vec2 spriteUnit;
    Tile elements[];
} tilesetSSBO;

layout(location = 0) out vec2 fragUVs;

ivec3 getTilePositionFromIndex(int index)
{
    index = clamp(index, 0, GRID_W * GRID_H * GRID_LAYER - 1);
    int perLayer = GRID_W * GRID_H;
    int layer    = index / perLayer;
    int inLayer  = index - layer * perLayer;
    int y = inLayer / GRID_W;
    int x = inLayer - y * GRID_W;
    return ivec3(x, y, layer);
}

void main()
{
    int cellIndex = chunkDataUBO.activeCells[gl_InstanceID];

    int  tileID   = chunkDataUBO.data[cellIndex];
    Tile tile     = tilesetSSBO.elements[tileID];
    ivec3 tilePos = getTilePositionFromIndex(cellIndex);

    ivec2 chunkBaseCells = chunkDataUBO.position * ivec2(GRID_W, GRID_H);
    ivec2 cellXY = chunkBaseCells + tilePos.xy;
    vec2  local = modelPosition;
    vec2  worldXY = vec2(cellXY) + local;
    float z = float(tilePos.z);

    vec4 worldPos = vec4(worldXY, z, 1.0);
    gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;

    vec2 baseUV = vec2(tile.coordinates) * tilesetSSBO.spriteUnit;
    vec2 frameOffsetUV = vec2(0.0);
    
	if (tile.nbFrames > 0 && tile.animationDuration > 0u)
	{
        uint t     = timeUBO.epoch % tile.animationDuration;
        uint step  = max(1u, tile.animationDuration / uint(tile.nbFrames));
        uint frame = min(t / step, uint(max(0, tile.nbFrames - 1)));
        ivec2 delta = tile.offsetBetweenFrames * int(frame);
        frameOffsetUV = vec2(delta) * tilesetSSBO.spriteUnit;
    }
    
	vec2 perVertexUV = local * tilesetSSBO.spriteUnit;
    fragUVs = baseUV + frameOffsetUV + perVertexUV;
}
