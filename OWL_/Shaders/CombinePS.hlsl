Texture2D g_texture0 : register(t0);
Texture2D g_texture1 : register(t1);
Texture2D g_prevFrame : register(t2);
SamplerState g_sampler : register(s0);

cbuffer ImageFilterConstData : register(b0)
{
    float dx;
    float dy;
    float threshold;
    float strength;
    float exposure; // option1 in c++
    float gamma; // option2 in c++
    float blur; // option3 in c++
    float option4;
};

struct SamplingPixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 FilmicToneMapping(float3 color)
{
    color = max(float3(0.0f, 0.0f, 0.0f), color);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    return color;
}

float3 LinearToneMapping(float3 color)
{
    float3 invGamma = float3(1.0f, 1.0f, 1.0f) / gamma;

    color = clamp(exposure * color, 0.0f, 1.0f);
    color = pow(color, invGamma);
    return color;
}

float3 Uncharted2ToneMapping(float3 color)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    float W = 11.2f;
    
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    color = pow(color, float3(1.0f, 1.0f, 1.0f) / gamma);
    return color;
}

float3 LumaBasedReinhardToneMapping(float3 color)
{
    float3 invGamma = float3(1.0f, 1.0f, 1.0f) / gamma;
    float luma = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float toneMappedLuma = luma / (1.0f + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, invGamma);
    return color;
}

float4 main(SamplingPixelShaderInput input) : SV_TARGET
{
    float3 color0 = g_texture0.Sample(g_sampler, input.texcoord).rgb;
    float3 color1 = g_texture1.Sample(g_sampler, input.texcoord).rgb;
    float3 combined = (1.0f - strength) * color0 + strength * color1;

    // Tone Mapping.
    combined = LinearToneMapping(combined);
    combined = lerp(combined, g_prevFrame.Sample(g_sampler, input.texcoord).rgb, blur);
    
    return float4(combined, 1.0f);
}
