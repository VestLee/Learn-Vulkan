#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "common.glsl"

layout (location = 0) in vec4 inPosition;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform _ShadowMapData {GPUShadowMapData shadowMapData;};

vec2 warp_depth(float depth, vec2 exponents)
{
    // Rescale depth into [-1, 1]
    depth = 2.0f * depth - 1.0f;
    float pos =  exp( exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return vec2(pos, neg);
}

void main()
{
    float depth = inPosition.z;
    vec2 exponents = vec2(shadowMapData.positiveExponent, shadowMapData.negativeExponent);
    vec2 vsmDepth = warp_depth(depth, exponents);
    outFragColor = vec4(vsmDepth.xy, vsmDepth.xy * vsmDepth.xy);
}