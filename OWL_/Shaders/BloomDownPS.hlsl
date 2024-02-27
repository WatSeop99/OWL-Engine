Texture2D g_Texture0 : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer SamplingPixelConstantData : register(b0)
{
    float DX;
    float DY;
    float Threshold;
    float Strength;
    float4 Options;
};

struct SamplingPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float2 Texcoord : TEXCOORD;
};

float4 main(SamplingPixelShaderInput input) : SV_TARGET
{
    float x = input.Texcoord.x;
    float y = input.Texcoord.y;
    
    float3 a = g_Texture0.Sample(g_Sampler, float2(x - 2.0f * DX, y + 2 * DY)).rgb;
    float3 b = g_Texture0.Sample(g_Sampler, float2(x, y + 2.0f * DY)).rgb;
    float3 c = g_Texture0.Sample(g_Sampler, float2(x + 2.0f * DX, y + 2.0f * DY)).rgb;

    float3 d = g_Texture0.Sample(g_Sampler, float2(x - 2.0f * DX, y)).rgb;
    float3 e = g_Texture0.Sample(g_Sampler, float2(x, y)).rgb;
    float3 f = g_Texture0.Sample(g_Sampler, float2(x + 2.0f * DX, y)).rgb;

    float3 g = g_Texture0.Sample(g_Sampler, float2(x - 2.0f * DX, y - 2.0f * DY)).rgb;
    float3 h = g_Texture0.Sample(g_Sampler, float2(x, y - 2.0f * DY)).rgb;
    float3 i = g_Texture0.Sample(g_Sampler, float2(x + 2.0f * DX, y - 2.0f * DY)).rgb;

    float3 j = g_Texture0.Sample(g_Sampler, float2(x - DX, y + DY)).rgb;
    float3 k = g_Texture0.Sample(g_Sampler, float2(x + DX, y + DY)).rgb;
    float3 l = g_Texture0.Sample(g_Sampler, float2(x - DX, y - DY)).rgb;
    float3 m = g_Texture0.Sample(g_Sampler, float2(x + DX, y - DY)).rgb;

    float3 color = e * 0.125f;
    color += (a + c + g + i) * 0.03125f;
    color += (b + d + f + h) * 0.0625f;
    color += (j + k + l + m) * 0.125f;
  
    return float4(color, 1.0f);
}