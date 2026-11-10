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
layout(location = 2) in vec3 worldNormal;
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

	const vec3 lightDirection = normalize(vec3(0.35, 0.85, 0.45));
	float illumination = 0.38 + 0.62 * max(dot(normalize(worldNormal), lightDirection), 0.0);
	outColor = vec4(albedo.rgb * illumination, 1.0);
}
