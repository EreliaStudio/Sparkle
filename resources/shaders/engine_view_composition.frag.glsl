#version 460 core

uniform sampler2D uOpaque;
uniform sampler2D uAccumulation;
uniform sampler2D uRevealage;
uniform float uExposure;

layout(location = 0) in vec2 vertexUV;
layout(location = 0) out vec4 outColor;

float reinhard(float value)
{
    float exposed = max(0.0, value) * uExposure;
    if (exposed < 1.0e8)
    {
        return exposed / (1.0 + exposed);
    }
    return 1.0 - 1.0 / (1.0 + exposed);
}

void main()
{
    vec3 opaque = texture(uOpaque, vertexUV).rgb;
    vec4 accumulated = texture(uAccumulation, vertexUV);
    float revealage = clamp(texture(uRevealage, vertexUV).r, 0.0, 1.0);
    vec3 transparentColor = accumulated.rgb / max(accumulated.a, 1.0e-6);
    vec3 hdr = opaque * revealage + transparentColor * (1.0 - revealage);
    outColor = vec4(reinhard(hdr.r), reinhard(hdr.g), reinhard(hdr.b), 1.0);
}
