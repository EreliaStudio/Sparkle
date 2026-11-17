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
	mat4 viewProjection[6];
	vec4 parameters;
	vec4 pointPositionNear;
	vec4 pointFarResolution;
};

layout(std430) readonly buffer ForwardShadows
{
	ShadowData shadows[];
};

uniform sampler2D uAlbedo;
uniform sampler2DShadow uShadowMaps[16];
uniform samplerCubeShadow uPointShadowMaps[8];

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

// OpenGL cube face order: +X, -X, +Y, -Y, +Z, -Z.
// Dominant-axis ties are explicitly resolved X > Y > Z.
int pointFace(vec3 direction)
{
	vec3 a = abs(direction);
	if (a.x >= a.y && a.x >= a.z)
	{
		return direction.x >= 0.0 ? 0 : 1;
	}
	if (a.y >= a.z)
	{
		return direction.y >= 0.0 ? 2 : 3;
	}
	return direction.z >= 0.0 ? 4 : 5;
}

vec2 pointFaceUV(vec3 direction, int face)
{
	if (face == 0) return vec2(-direction.z, -direction.y) / abs(direction.x);
	if (face == 1) return vec2( direction.z, -direction.y) / abs(direction.x);
	if (face == 2) return vec2( direction.x,  direction.z) / abs(direction.y);
	if (face == 3) return vec2( direction.x, -direction.z) / abs(direction.y);
	if (face == 4) return vec2( direction.x, -direction.y) / abs(direction.z);
	return vec2(-direction.x, -direction.y) / abs(direction.z);
}

vec3 pointFaceDirection(int face, vec2 uv)
{
	if (face == 0) return vec3( 1.0, -uv.y, -uv.x);
	if (face == 1) return vec3(-1.0, -uv.y,  uv.x);
	if (face == 2) return vec3( uv.x,  1.0,  uv.y);
	if (face == 3) return vec3( uv.x, -1.0, -uv.y);
	if (face == 4) return vec3( uv.x, -uv.y,  1.0);
	return vec3(-uv.x, -uv.y, -1.0);
}

float pointComparison(ShadowData shadow, int slot, vec3 ray, vec3 worldRay, vec3 receiver)
{
	int face = pointFace(ray);
	// Check the physical, unshifted receiver before applying normal bias.
	// Otherwise an arbitrarily close fragment could sample past near.
	if (abs(worldRay[face / 2]) < shadow.pointPositionNear.w)
	{
		return 1.0;
	}
	vec4 clip = shadow.viewProjection[face] * vec4(receiver, 1.0);
	if (clip.w <= 0.0)
	{
		return 1.0;
	}
	vec3 projected = clip.xyz / clip.w * 0.5 + 0.5;
	if (clip.w > shadow.pointFarResolution.x ||
		projected.z <= 0.0 || projected.z >= 1.0)
	{
		return 1.0;
	}
	// The cube hardware must make the same deterministic choice at exact
	// dominant-axis ties. One positive float ULP breaks those ties.
	vec3 direction = ray / max(max(abs(ray.x), abs(ray.y)), abs(ray.z));
	int axis = face / 2;
	float competing = axis == 0 ? max(abs(direction.y), abs(direction.z)) :
		(axis == 1 ? max(abs(direction.x), abs(direction.z)) :
		max(abs(direction.x), abs(direction.y)));
	if (abs(direction[axis]) == competing)
	{
		direction[axis] = sign(direction[axis]) *
			uintBitsToFloat(floatBitsToUint(competing) + 1u);
	}
	return texture(uPointShadowMaps[slot], vec4(direction, projected.z - shadow.parameters.x));
}

float pointShadowVisibility(ShadowData shadow, int slot, vec3 position, vec3 normal)
{
	vec3 ray = position - shadow.pointPositionNear.xyz;
	if (all(equal(ray, vec3(0.0))))
	{
		return 1.0;
	}
	float maximum = max(max(abs(ray.x), abs(ray.y)), abs(ray.z));
	vec3 q = ray / maximum;
	// Do not normalize a tiny world-space vector directly.
	vec3 stableRay = q / sqrt(dot(q, q));
	vec3 receiver = position + normal * shadow.parameters.y;
	if (shadow.pointFarResolution.z == 0.0)
	{
		return pointComparison(shadow, slot, stableRay, ray, receiver);
	}
	int centralFace = pointFace(stableRay);
	vec2 uv = pointFaceUV(stableRay, centralFace);
	float texel = 2.0 / shadow.pointFarResolution.y;
	float visibility = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec3 direction = pointFaceDirection(centralFace, uv + vec2(x, y) * texel);
			visibility += pointComparison(shadow, slot, direction, ray, receiver);
		}
	}
	return visibility / 9.0;
}

float shadowVisibility(uint i, vec3 position, vec3 normal)
{
	ShadowData shadow = shadows[i];
	int slot = int(shadow.parameters.w) - 1;
	if (slot < 0)
	{
		return 1.0;
	}
	if (shadow.pointFarResolution.w != 0.0)
	{
		return slot < 8 ? pointShadowVisibility(shadow, slot, position, normal) : 1.0;
	}
	if (slot >= 16)
	{
		return 1.0;
	}
	vec3 receiver = position + normal * shadow.parameters.y;
	vec4 clip = shadow.viewProjection[0] * vec4(receiver, 1.0);
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
	float bias = shadow.parameters.x;
	if (shadow.parameters.z == 0.0)
	{
		return texture(uShadowMaps[slot], vec3(projected.xy, projected.z - bias));
	}
	float visibility = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 sampleUV = projected.xy + vec2(x, y) * shadow.parameters.z;
			visibility += texture(uShadowMaps[slot], vec3(sampleUV, projected.z - bias));
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
			attenuation *= shadowVisibility(i, worldPosition, normal);
		}
		illumination += light.colorIntensity.rgb * (light.colorIntensity.w * attenuation * lambert);
	}
	outColor = vec4(albedo.rgb * illumination, 1.0);
}
