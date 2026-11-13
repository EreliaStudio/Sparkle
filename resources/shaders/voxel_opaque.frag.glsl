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

layout(std430) readonly buffer MaterialRemap
{
	uint paletteIndices[];
};

struct LightData
{
	vec4 positionRange;
	vec4 directionCosine;
	vec4 colorIntensity;
	uvec4 typeAndPadding;
};

layout(std430) readonly buffer ForwardLighting
{
	vec4 ambientRGB;
	uvec4 lightCounts;
	LightData lights[];
};

uniform sampler2D uAlbedo;

layout(location = 0) in vec2 vertexUV;
layout(location = 1) flat in uint materialIndex;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec3 worldPosition;
layout(location = 0) out vec4 outColor;

float radialAttenuation(float distance, float range)
{
	if (range <= 0.0 || distance >= range)
	{
		return 0.0;
	}
	float x = clamp(distance / range, 0.0, 1.0);
	return 1.0 - 3.0 * x * x + 2.0 * x * x * x;
}

float spotAttenuation(vec3 direction, vec3 toReceiver, float cosineEdge)
{
	float cosine = dot(normalize(direction), toReceiver);
	float t = clamp((cosine - cosineEdge) / (1.0 - cosineEdge), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

void main()
{
	MaterialSection section = sections[paletteIndices[materialIndex]];
	vec2 atlasSize = vec2(textureSize(uAlbedo, 0));
	vec2 firstTexel = floor(section.anchor * atlasSize);
	vec2 lastTexel = max(firstTexel, ceil((section.anchor + section.size) * atlasSize) - 1.0);
	vec2 uv = clamp(
		section.anchor + vertexUV * section.size,
		(firstTexel + 0.5) / atlasSize,
		(lastTexel + 0.5) / atlasSize);
	vec4 albedo = texture(uAlbedo, uv);
	if (albedo.a <= 0.0)
	{
		discard;
	}

	vec3 normal = normalize(worldNormal);
	vec3 illumination = ambientRGB.rgb;
	for (uint i = 0u; i < lightCounts.x; ++i)
	{
		LightData light = lights[i];
		if (light.colorIntensity.w <= 0.0)
		{
			continue;
		}

		float attenuation = 1.0;
		float lambert = 0.0;
		if (light.typeAndPadding.x == 2u)
		{
			lambert = max(dot(normal, -normalize(light.directionCosine.xyz)), 0.0);
		}
		else
		{
			vec3 offset = light.positionRange.xyz - worldPosition;
			float distance = length(offset);
			attenuation = radialAttenuation(distance, light.positionRange.w);
			if (attenuation <= 0.0)
			{
				continue;
			}
			vec3 toLight = distance > 0.0 ? offset / distance : vec3(0.0);
			lambert = distance > 0.0 ? max(dot(normal, toLight), 0.0) : 1.0;
			if (light.typeAndPadding.x == 1u && distance > 0.0)
			{
				attenuation *= spotAttenuation(light.directionCosine.xyz, -toLight, light.directionCosine.w);
			}
		}
		illumination += light.colorIntensity.rgb * (light.colorIntensity.w * attenuation * lambert);
	}
	outColor = vec4(albedo.rgb * illumination, 1.0);
}
