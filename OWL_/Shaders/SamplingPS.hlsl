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
    float4 ScreenPosition : SV_POSITION;
    float2 Texcoord : TEXCOORD;
};

float4 main(SamplingPixelShaderInput input) : SV_TARGET
{
    float3 color = g_Texture0.Sample(g_Sampler, input.Texcoord).rgb;
    float l = (color.x + color.y + color.y) / 3;
    
    if (l > Threshold)
    {
        return float4(color, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}