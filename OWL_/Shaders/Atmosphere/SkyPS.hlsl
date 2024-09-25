#include "PostColor.hlsli"

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

static const float PI = 3.14159265f;

cbuffer PSTransform : register(b0)
{
    float3 g_FrustumA;
    float pad0;
    float3 g_FrustumB;
    float pad1;
    float3 g_FrustumC;
    float pad2;
    float3 g_FrustumD;
    float pad3;
}

Texture2D<float3> g_SkyView : register(t0);

SamplerState g_SkyViewSampler : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 dir = normalize(lerp(lerp(g_FrustumA, g_FrustumB, input.TexCoord.x), lerp(g_FrustumC, g_FrustumD, input.TexCoord.x), input.TexCoord.y));

    float phi = atan2(dir.z, dir.x);
    float u = phi / (2 * PI);
    
    float theta = asin(dir.y);
    float v = 0.5f + 0.5f * sign(theta) * sqrt(abs(theta) / (PI * 0.5f));

    float3 skyColor = g_SkyView.SampleLevel(g_SkyViewSampler, float2(u, v), 0.0f);

    skyColor = PostProcessColor(input.TexCoord, skyColor);
    return float4(skyColor, 1.0f);
}
