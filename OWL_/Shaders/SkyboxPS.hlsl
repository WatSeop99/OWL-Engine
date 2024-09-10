#include "Common.hlsli"

struct SkyboxPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 ModelPosition : POSITION;
};

struct PixelShaderOutput
{
    float4 PixelColor : SV_TARGET0;
};

PixelShaderOutput main(SkyboxPixelShaderInput input)
{
    PixelShaderOutput output;
    
    // output.PixelColor = g_EnvIBLTex.SampleLevel(g_LinearWrapSampler, input.ModelPosition.xyz, g_EnvLodBias);
    switch (g_TextureToDraw)
    {
        case 0:
            output.PixelColor = g_EnvIBLTex.SampleLevel(g_LinearWrapSampler, input.ModelPosition.xyz, g_EnvLodBias);
            break;

        case 1:
            output.PixelColor = g_SpecularIBLTex.SampleLevel(g_LinearWrapSampler, input.ModelPosition.xyz, g_EnvLodBias);
            break;
        
        case 2:
            output.PixelColor = g_IrradianceIBLTex.SampleLevel(g_LinearWrapSampler, input.ModelPosition.xyz, g_EnvLodBias);
            break;

        default:
            output.PixelColor = float4(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);
            break;
    }

    output.PixelColor.rgb *= g_StrengthIBL;
    output.PixelColor.a = 1.0f;
    
    return output;
}
