#include "TileableNoise.hlsli"

cbuffer TerrainConstants : register(b0)
{
    float g_Scale;
    int g_Octaves;
    float g_Persistance;
    float g_Lacunarity;
    float g_MinHeight;
    float g_MaxHeight;
};

StructuredBuffer<float2> g_OctaveOffset : register(t0);

RWTexture2D<float> g_HeightTexture : register(u0);
RWTexture2D<float4> g_ColorTexture : register(u1);

float GetRandomNoise(float2 uv)
{
    float freq = 4.0f;

    float pfbm = lerp(1.0f, Perlinfbm(float3(uv, 0.0f), 4.0f, 7.0f), 0.5f);
    pfbm = abs(pfbm * 2.0f - 1.0f); // billowy perlin noise

    float wfbm = WorleyFbm(float3(uv, 0.0f), freq);
    
    float ret = Remap(pfbm, 0.0f, 1.0f, wfbm, 1.0f);
    
    return ret;
}

float4 GetColor(float height)
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (height > 0.8f)
    {
        color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (height <= 0.8f)
    {
        color = float4(0.4f, 0.2f, 0.0f, 1.0f);
    }
    if (height <= 0.7f)
    {
        color = float4(0.6f, 0.2980392156862745f, 0.0f, 1.0f);
    }
    if (height <= 0.6f)
    {
        color = float4(0.0f, 0.4f, 0.2f, 1.0f);
    }
    if (height <= 0.55f)
    {
        color = float4(0.0f, 0.8f, 0.0f, 1.0f);
    }
    if (height <= 0.45f)
    {
        color = float4(1.0f, 1.0f, 0.6f, 1.0f);
    }
    if (height <= 0.4f)
    {
        color = float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    if (height <= 0.3f)
    {
        color = float4(0.0f, 0.0f, 0.6f, 1.0f);
    }

    return color;
}

[numthreads(16, 16, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    uint width, height;
    g_HeightTexture.GetDimensions(width, height);

    float2 uv = dtID.xy / float2(width, height);

    float amplitude = 1.0f;
    float frequency = 1.0f;
    float noiseHeight = 0.0f;
    uint halfWidth = width / 2;
    uint halfHeight = height / 2;

    [loop]
    for (int i = 0; i < g_Octaves; ++i)
    {
        float2 octaveOffset = g_OctaveOffset[i];
        float sampleX = (uv.x - halfWidth) / g_Scale * frequency + octaveOffset.x;
        float sampleY = (uv.y - halfHeight) / g_Scale * frequency + octaveOffset.y;

        float perlinValue = GetRandomNoise(float2(sampleX, sampleY)) * 2.0f - 1.0f;
        noiseHeight += perlinValue * amplitude;

        amplitude *= g_Persistance;
        frequency *= g_Lacunarity;
    }

    //float min_input = 0.0f;
    //float max_input = 1.0f;
    //float min_output = -100.0f;
    //float max_output = 100.0f;
    //float mapped_value = min_output + (input_value - min_input) * (max_output - min_output) / (max_input - min_input);
    //return mapped_value;
    
    g_ColorTexture[dtID.xy] = GetColor(noiseHeight);
    //g_HeightTexture[dtID.xy] = g_MinHeight + noiseHeight * (g_MaxHeight - g_MinHeight);
    g_HeightTexture[dtID.xy] = clamp(noiseHeight, g_MinHeight, g_MaxHeight);
}