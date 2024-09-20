#ifndef SHADING_UTIL
#define SHADING_UTIL

#include "DiskSamples.hlsli"

// #define LIGHT_WORLD_RADIUS 0.001f
//#define LIGHT_FRUSTUM_WIDTH 0.34641f // <- ����ؼ� ã�� ��
// #define LIGHT_FRUSTUM_WIDTH 0.2f
static const float LIGHT_FRUSTUM_WIDTH = 0.34641f;
static const float NEAR_PLANE = 0.01f;
static const float FAR_PLANE = 50.0f;

static const float3 F_DIELECTRIC = 0.04f; // ��ݼ�(Dielectric) ������ F0.

// NdcDepthToViewDepth.
float N2V(float ndcDepth, matrix invProj)
{
    // return invProj[3][2] / (ndcDepth - invProj[2][2]);
    float4 pointView = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), invProj);
    return pointView.z / pointView.w;
}

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
    const float3 Fdielectric = 0.04f; // ��ݼ�(Dielectric) ������ F0
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



float PCFFilterSpotLight(Texture2D shadowMap, SamplerComparisonState shadowCompare, float2 uv, float zReceiverNDC, float filterRadiusUV)
{
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(shadowCompare, uv + offset, zReceiverNDC);
    }
    return sum / 64.0f;
}

float PCFFilterDirectionalLight(Texture2DArray shadowMap, SamplerComparisonState shadowCompare, int shadowMapIndex, float2 uv, float zReceiverNDC, float filterRadiusUV)
{
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(shadowCompare, float3(uv + offset, shadowMapIndex), zReceiverNDC);
    }
    return sum / 64.0f;
}

float PCFFilterPointLight(TextureCube shadowMap, SamplerComparisonState shadowCompare, float3 uvw, float zReceiverNDC, float filterRadiusUV)
{
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float3 offset = float3(diskSamples64[i], 0.0f) * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(shadowCompare, uvw + offset, zReceiverNDC);
    }
    return sum / 64.0f;
}

void FindBlockerInSpotLight(out float avgBlockerDepthView, out float numBlockers, Texture2D shadowMap, SamplerState shadowPoint, float2 uv, float zReceiverView, matrix inverseProjection, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float searchRadius = lightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView;

    float blockerSum = 0.0f;
    numBlockers = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float shadowMapDepth = shadowMap.SampleLevel(shadowPoint, float2(uv + diskSamples64[i] * searchRadius), 0.0f).r;
        shadowMapDepth = N2V(shadowMapDepth, inverseProjection);
        
        if (shadowMapDepth < zReceiverView)
        {
            blockerSum += shadowMapDepth;
            ++numBlockers;
        }
    }
    
    avgBlockerDepthView = blockerSum / numBlockers;
}

void FindBlockerInDirectionalLight(out float avgBlockerDepthView, out float numBlockers, Texture2DArray shadowMap, SamplerState shadowPoint, float2 uv, float zReceiverView, int shadowMapIndex, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float searchRadius = lightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView;

    float blockerSum = 0.0f;
    numBlockers = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float shadowMapDepth = shadowMap.SampleLevel(shadowPoint, float3(uv + diskSamples64[i] * searchRadius, shadowMapIndex), 0.0f).r;
        shadowMapDepth = N2V(shadowMapDepth, invProj);
        
        if (shadowMapDepth < zReceiverView)
        {
            blockerSum += shadowMapDepth;
            ++numBlockers;
        }
    }
    
    avgBlockerDepthView = blockerSum / numBlockers;
}

void FindBlockerInPointLight(out float avgBlockerDepthView, out float numBlockers, TextureCube shadowMap, SamplerState shadowPoint, float3 uvw, float zReceiverView, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float searchRadius = lightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView;

    float blockerSum = 0.0f;
    numBlockers = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float shadowMapDepth = shadowMap.SampleLevel(shadowPoint, uvw + float3(diskSamples64[i], 0.0f) * searchRadius, 0.0f).r;
        shadowMapDepth = N2V(shadowMapDepth, invProj);
        
        if (shadowMapDepth < zReceiverView)
        {
            blockerSum += shadowMapDepth;
            ++numBlockers;
        }
    }
    
    avgBlockerDepthView = blockerSum / numBlockers;
}

float PCSSForSpotLight(Texture2D shadowMap, SamplerState shadowPoint, SamplerComparisonState shadowCompare, float2 uv, float zReceiverNDC, matrix inverseProjection, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float zReceiverView = N2V(zReceiverNDC, inverseProjection);
    
    // STEP 1: blocker search.
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    FindBlockerInSpotLight(avgBlockerDepthView, numBlockers, shadowMap, shadowPoint, uv, zReceiverView, inverseProjection, lightRadiusWorld);

    if (numBlockers < 1)
    {
        // There are no occluders so early out(this saves filtering).
        return 1.0f;
    }
    else
    {
        // STEP 2: penumbra size.
        float penumbraRatio = (zReceiverView - avgBlockerDepthView) / avgBlockerDepthView;
        float filterRadiusUV = penumbraRatio * lightRadiusUV * NEAR_PLANE / zReceiverView;

        // STEP 3: filtering.
        return PCFFilterSpotLight(shadowMap, shadowCompare, uv, zReceiverNDC, filterRadiusUV);
    }
}

float PCSSForDirectionalLight(Texture2DArray shadowMap, SamplerState shadowPoint, SamplerComparisonState shadowCompare, int shadowMapIndex, float2 uv, float zReceiverNDC, matrix inverseProjection, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float zReceiverView = N2V(zReceiverNDC, inverseProjection);
    
    // STEP 1: blocker search.
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    FindBlockerInDirectionalLight(avgBlockerDepthView, numBlockers, shadowMap, shadowPoint, uv, zReceiverView, shadowMapIndex, inverseProjection, lightRadiusWorld);

    if (numBlockers < 1)
    {
        // There are no occluders so early out(this saves filtering).
        return 1.0f;
    }
    else
    {
        // STEP 2: penumbra size.
        float penumbraRatio = (zReceiverView - avgBlockerDepthView) / avgBlockerDepthView;
        float filterRadiusUV = penumbraRatio * lightRadiusUV * NEAR_PLANE / zReceiverView;

        // STEP 3: filtering.
        return PCFFilterDirectionalLight(shadowMap, shadowCompare, shadowMapIndex, uv, zReceiverNDC, filterRadiusUV);
    }
}

float PCSSForPointLight(TextureCube shadowMap, SamplerState shadowPoint, SamplerComparisonState shadowCompare, float3 uvw, float zReceiverNDC, matrix inverseProjection, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float zReceiverView = N2V(zReceiverNDC, inverseProjection);
    // float zReceiverView = length(uvw);
    
    // STEP 1: blocker search.
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    FindBlockerInPointLight(avgBlockerDepthView, numBlockers, shadowMap, shadowPoint, uvw, zReceiverView, inverseProjection, lightRadiusWorld);

    if (numBlockers < 1)
    {
        // There are no occluders so early out(this saves filtering).
        return 1.0f;
    }
    else
    {
        // STEP 2: penumbra size.
        float penumbraRatio = (zReceiverView - avgBlockerDepthView) / avgBlockerDepthView;
        float filterRadiusUV = penumbraRatio * lightRadiusUV * NEAR_PLANE / zReceiverView;

        // STEP 3: filtering.
        return PCFFilterPointLight(shadowMap, shadowCompare, uvw, zReceiverNDC, filterRadiusUV);
    }
}

#endif