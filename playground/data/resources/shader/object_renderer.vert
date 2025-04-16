#version 450 core

layout(location = 0) in vec2 vertexPosition;

layout(std140, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 projection;
};

layout(std430, binding = 1) buffer BodyList
{
	vec2 offsets[];
};

out vec4 fragColor;

void main()
{
	vec2 worldPosition = vertexPosition + offsets[gl_InstanceID];
    gl_Position = projection * view * vec4(worldPosition, 0.0, 1.0);

    fragColor = vec4(1, 0, 0, 1);
}