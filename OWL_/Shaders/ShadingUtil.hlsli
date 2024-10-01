#ifndef SHADING_UTIL
#define SHADING_UTIL

#include "ShadowUtil.hlsli"

static const float3 F_DIELECTRIC = 0.04f; // 비금속(Dielectric) 재질의 F0.

float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1.0f - F0) * pow(2.0f, (-5.55473f * NdotH - 6.98316f) * NdotH);
    //return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 DiffuseIBL(TextureCube irradianceIBLTex, SamplerState linearWarp, float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic)
{
    float3 F0 = lerp(F_DIELECTRIC, albedo, metallic);
    float3 F = SchlickFresnel(F0, max(0.0f, dot(normalWorld, pixelToEye)));
    float3 kd = lerp(1.0f - F, 0.0f, metallic);
    float3 irradiance = irradianceIBLTex.SampleLevel(linearWarp, normalWorld, 0.0f).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(Texture2D brdfTex, TextureCube specularIBLTex, SamplerState linearClamp, SamplerState linearWrap, float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic, float roughness)
{
    float2 specularBRDF = brdfTex.SampleLevel(linearClamp, float2(dot(normalWorld, pixelToEye), 1.0f - roughness), 0.0f).rg;
    float3 specularIrradiance = specularIBLTex.SampleLevel(linearWrap, reflect(-pixelToEye, normalWorld), 2.0f + roughness * 5.0f).rgb;
    const float3 Fdielectric = 0.04f; // 비금속(Dielectric) 재질의 F0
    float3 F0 = lerp(Fdielectric, albedo, metallic);

    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
}

float3 AmbientLightingByIBL(TextureCube idrradianceIBLTex, TextureCube specularIBLTex, Texture2D brdfTex, SamplerState linearClamp, SamplerState linearWrap,
    float3 albedo, float3 normalW, float3 pixelToEye, float ao, float metallic, float roughness)
{
    float3 diffuseIBL = DiffuseIBL(idrradianceIBLTex, linearWrap, albedo, normalW, pixelToEye, metallic);
    float3 specularIBL = SpecularIBL(brdfTex, specularIBLTex, linearClamp, linearWrap, albedo, normalW, pixelToEye, metallic, roughness);
    
    return (diffuseIBL + specularIBL) * ao;
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NdfGGX(float NdotH, float roughness, float alphaPrime)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0f) + 1.0f;
    return alphaPrime * alphaPrime / (3.141592f * denom * denom);
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float SchlickGGX(float NdotI, float NdotO, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

#endif