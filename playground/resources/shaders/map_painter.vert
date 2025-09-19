#version 450

layout (location = 0) in vec2 modelPosition;

layout(std140, binding = 0) uniform CameraUBO
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} cameraUBO;

const int GRID_W = 16;
const int GRID_H = 16;
const int CELL_PER_VALUE = 4;

layout(std140, binding = 1) uniform ChunkDataUBO
{
	ivec2 position;
    uint data[GRID_W * GRID_H / CELL_PER_VALUE];
} chunkDataUBO;

layout(std430, binding = 2) buffer ColorPaletteSSBO
{
    uint hexCodes[];
} colorPaletteSSBO;

layout (location = 0) out flat vec4 vColor;

uint getColorIdentifier(int x, int y)
{
    int i = y * GRID_W + x;

    int word = i >> 2;

    int byteInWord = i & 3;

    uint packed = chunkDataUBO.data[word];

    uint shift = uint(byteInWord * 8);

    return (packed >> shift) & 0xFFu;
}

vec4 decodeRGBA8(uint hexCode)
{
    float r = float((hexCode >>  0) & 255u) / 255.0;
    float g = float((hexCode >>  8) & 255u) / 255.0;
    float b = float((hexCode >> 16) & 255u) / 255.0;
    float a = float((hexCode >> 24) & 255u) / 255.0;
    return vec4(r, g, b, a);
}

void main()
{
    int x =  gl_InstanceID & 15; // translate to "% 16"
	int y =  gl_InstanceID >> 4; // translate to "/ 16"

    uint colorId = getColorIdentifier(x, y);
    uint hexCode = colorPaletteSSBO.hexCodes[colorId];
    vColor = decodeRGBA8(hexCode);

	vec2 chunkWorldPos = vec2(chunkDataUBO.position) * vec2(GRID_W, GRID_H);
    vec2 worldXY = modelPosition + vec2(x, y) + chunkWorldPos;
    vec4 worldPos = vec4(worldXY, 0.0, 1.0);

    gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * worldPos;
}
