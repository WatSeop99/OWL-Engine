#include "AtmosphereUtil.hlsli"

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float4 ClipPos : CLIP_POSITION;
    float3 Transmittance : TRANSMITTANCE;
};

cbuffer SunVSConstants : register(b0)
{
    matrix g_WVP;

    float g_SunTheta;
    float g_EyeHeight;
}

Texture2D<float3> g_Transmittance : register(t0);

SamplerState g_TransmittanceSampler : register(s0);

PixelShaderInput main(float2 position : POSITION)
{
    PixelShaderInput output;
    output.Position = mul(float4(position, 0.0f, 1.0f), g_WVP);
    output.Position.z = output.Position.w;
    output.ClipPos = output.Position;
    output.Transmittance = GetTransmittance(g_Transmittance, g_TransmittanceSampler, g_EyeHeight, g_SunTheta);

    return output;
}
