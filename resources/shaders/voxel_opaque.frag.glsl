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

struct ShadowData
{
	mat4 viewProjection;
	vec4 parameters;
};

layout(std430) readonly buffer ForwardShadows
{
	ShadowData shadows[];
};

uniform sampler2D uAlbedo;
uniform sampler2D uShadowMaps[16];

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

float shadowVisibility(uint i, vec3 position, vec3 normal, vec3 toLight)
{
	ShadowData shadow = shadows[i];
	int slot = int(shadow.parameters.w) - 1;
	if (slot < 0 || slot >= 16)
	{
		return 1.0;
	}
	vec4 clip = shadow.viewProjection * vec4(position, 1.0);
	if (clip.w <= 0.0)
	{
		return 1.0;
	}
	vec3 projected = clip.xyz / clip.w * 0.5 + 0.5;
	if (projected.x < 0.0 || projected.x > 1.0 ||
		projected.y < 0.0 || projected.y > 1.0 ||
		projected.z <= 0.0 || projected.z >= 1.0)
	{
		return 1.0;
	}
	float bias = shadow.parameters.x + shadow.parameters.y *
		(1.0 - max(dot(normal, normalize(toLight)), 0.0));
	float visibility = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 sampleUV = projected.xy + vec2(x, y) * shadow.parameters.z;
			float depth = texture(uShadowMaps[slot], sampleUV).r;
			visibility += projected.z - bias <= depth ? 1.0 : 0.0;
		}
	}
	return visibility / 9.0;
}

void main()
{
	MaterialSection section = sections[paletteIndices[materialIndex]];
	vec2 uv = section.anchor + vertexUV * section.size;
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
		vec3 lightVector = -normalize(light.directionCosine.xyz);
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
			lightVector = toLight;
			lambert = distance > 0.0 ? max(dot(normal, toLight), 0.0) : 1.0;
			if (light.typeAndPadding.x == 1u && distance > 0.0)
			{
				attenuation *= spotAttenuation(light.directionCosine.xyz, -toLight, light.directionCosine.w);
			}
		}
		if (attenuation > 0.0 && lambert > 0.0)
		{
			attenuation *= shadowVisibility(i, worldPosition, normal, lightVector);
		}
		illumination += light.colorIntensity.rgb * (light.colorIntensity.w * attenuation * lambert);
	}
	outColor = vec4(albedo.rgb * illumination, 1.0);
}
