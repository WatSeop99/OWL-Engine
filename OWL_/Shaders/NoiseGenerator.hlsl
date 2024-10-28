#include "TileableNoise.hlsli"

RWTexture2D<float> g_NoiseTexture : register(u0);

float GetRandomNoise(float2 uv)
{
    float freq = 4.0f;

    float pfbm = lerp(1.0f, Perlinfbm(float3(uv, 0.0f), 4.0f, 7.0f), 0.5f);
    pfbm = abs(pfbm * 2.0f - 1.0f); // billowy perlin noise

    float wfbm = WorleyFbm(float3(uv, 0.0f), freq);
    
    float ret = Remap(pfbm, 0.0f, 1.0f, wfbm, 1.0f);
    
    return ret;
}

[numthreads(16, 16, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint width, height;
    g_NoiseTexture.GetDimensions(width, height);

    float2 uv = dtID.xy / float2(width, height);

    g_NoiseTexture[dtID.xy] = GetRandomNoise(uv);
}