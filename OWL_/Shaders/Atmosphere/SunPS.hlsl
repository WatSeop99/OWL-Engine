#include "PostColor.hlsli"

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float4 ClipPos : CLIP_POSITION;
    float3 Transmittance : TRANSMITTANCE;
};

cbuffer SunPSConstants : register(b0)
{
    float3 g_SunRadiance;
    float pad;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(PostProcessColor(input.ClipPos.xy / input.ClipPos.w, input.Transmittance * g_SunRadiance), 1.0f);
}