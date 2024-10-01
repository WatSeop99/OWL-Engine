struct SamplingPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float2 Texcoord : TEXCOORD;
};


cbuffer SamplingPixelConstantData : register(b0)
{
    float DX;
    float DY;
    float Threshold;
    float Strength;
    float4 Options;
};

Texture2D g_Texture0 : register(t0);

SamplerState g_Sampler : register(s0);

float4 main(SamplingPixelShaderInput input) : SV_TARGET
{
    float x = input.Texcoord.x;
    float y = input.Texcoord.y;
    
    float3 a = g_Texture0.Sample(g_Sampler, float2(x - DX, y + DY)).rgb;
    float3 b = g_Texture0.Sample(g_Sampler, float2(x, y + DY)).rgb;
    float3 c = g_Texture0.Sample(g_Sampler, float2(x + DX, y + DY)).rgb;

    float3 d = g_Texture0.Sample(g_Sampler, float2(x - DX, y)).rgb;
    float3 e = g_Texture0.Sample(g_Sampler, float2(x, y)).rgb;
    float3 f = g_Texture0.Sample(g_Sampler, float2(x + DX, y)).rgb;

    float3 g = g_Texture0.Sample(g_Sampler, float2(x - DX, y - DY)).rgb;
    float3 h = g_Texture0.Sample(g_Sampler, float2(x, y - DY)).rgb;
    float3 i = g_Texture0.Sample(g_Sampler, float2(x + DX, y - DY)).rgb;

    float3 color = e * 4.0f;
    color += (b + d + f + h) * 2.0f;
    color += (a + c + g + i);
    color *= 1.0 / 16.0f;
  
    return float4(color, 1.0f);
}
