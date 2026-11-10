#version 460 core

struct MaterialSection
{
	vec2 anchor;
	vec2 size;
};

layout(std430) readonly buffer Materials
{
	MaterialSection sections[];
};

uniform sampler2D uAlbedo;

layout(location = 0) in vec2 vertexUV;
layout(location = 1) flat in uint materialIndex;
layout(location = 0) out vec4 outColor;

void main()
{
	MaterialSection section = sections[materialIndex];
	vec2 uv = section.anchor + vertexUV * section.size;
	vec4 albedo = texture(uAlbedo, uv);
	if (albedo.a <= 0.0)
	{
		discard;
	}
	outColor = vec4(albedo.rgb, 1.0);
}
