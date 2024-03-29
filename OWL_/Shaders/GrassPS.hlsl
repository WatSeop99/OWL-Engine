#include "Common.hlsli"

struct GrassPixelInput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 WorldNormal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 BaseColor : COLOR;
};

float4 main(GrassPixelInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(0.2f, 1.0f, 0.0f));
    
    // 간단한 directional light, 양면이라서 abs 사용.
    float3 color = input.BaseColor * abs(dot(input.WorldNormal, lightDir)) * 2.0f;
    return float4(color, 1.0f);
}

/*
float4 main(GrassPixelInput input) : SV_TARGET
{
    float3 pixelToEye = normalize(eyeWorld - input.posWorld);
    
    float3 normalWorld = dot(pixelToEye, input.normalWorld) > 0 ? input.normalWorld : -input.normalWorld; // 양면
    
    //float3 lightDir = normalize(float3(0.2, 1.0, 0.0));
    float3 irradiance = irradianceIBLTex.SampleLevel(linearWrapSampler, normalWorld, 0).rgb;
    
    // 간단한 directional light, 양면이라서 abs 사용
    float3 color = input.baseColor * irradiance * dot(pixelToEye, normalWorld) * strengthIBL;
    return float4(color, 1);
}
*/