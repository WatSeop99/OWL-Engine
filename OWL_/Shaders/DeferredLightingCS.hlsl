#include "Common.hlsli"
#include "DiskSamples.hlsli"

Texture2D<float4> g_AlbedoTex : register(t0);
Texture2D<float4> g_NormalTex : register(t1);
Texture2D<float4> g_WorldPositionTex : register(t2);
Texture2D<float4> g_EmissionTex : register(t3);
Texture2D<float4> g_ExtraTex : register(t4); // { metallic, roughness, ao, height }

RWTexture2D<unorm float4> g_OutputTexture : register(u0);

static const float3 F_DIELECTRIC = 0.04f; // 비금속(Dielectric) 재질의 F0.

float3 SchlickFresnel(float3 F0, float NdotH)
{
    return F0 + (1.0f - F0) * pow(2.0f, (-5.55473f * NdotH - 6.98316f) * NdotH);
    //return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 DiffuseIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic)
{
    float3 F0 = lerp(F_DIELECTRIC, albedo, metallic);
    float3 F = SchlickFresnel(F0, max(0.0f, dot(normalWorld, pixelToEye)));
    float3 kd = lerp(1.0f - F, 0.0f, metallic);
    float3 irradiance = irradianceIBLTex.SampleLevel(linearWrapSampler, normalWorld, 0.0f).rgb;
    
    return kd * albedo * irradiance;
}

float3 SpecularIBL(float3 albedo, float3 normalWorld, float3 pixelToEye, float metallic, float roughness)
{
    float2 specularBRDF = brdfTex.SampleLevel(linearClampSampler, float2(dot(normalWorld, pixelToEye), 1.0 - roughness), 0.0f).rg;
    float3 specularIrradiance = specularIBLTex.SampleLevel(linearWrapSampler, reflect(-pixelToEye, normalWorld),
                                                            2 + roughness * 5.0f).rgb;
    const float3 Fdielectric = 0.04f; // 비금속(Dielectric) 재질의 F0
    float3 F0 = lerp(Fdielectric, albedo, metallic);

    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
}

float3 AmbientLightingByIBL(float3 albedo, float3 normalW, float3 pixelToEye, float ao, float metallic, float roughness)
{
    float3 diffuseIBL = DiffuseIBL(albedo, normalW, pixelToEye, metallic);
    float3 specularIBL = SpecularIBL(albedo, normalW, pixelToEye, metallic, roughness);
    
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

// 참고: https://github.com/opengl-tutorials/ogl/blob/master/tutorial16_shadowmaps/ShadowMapping.fragmentshader
float random(float3 seed, int i)
{
    float4 seed4 = float4(seed, i);
    float dotProduct = dot(seed4, float4(12.9898f, 78.233f, 45.164f, 94.673f));
    return frac(sin(dotProduct) * 43758.5453f);
}

// NdcDepthToViewDepth.
float N2V(float ndcDepth, matrix invProj)
{
    float4 pointView = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), invProj);
    return pointView.z / pointView.w;
}

#define NEAR_PLANE 0.1f
// #define LIGHT_WORLD_RADIUS 0.001f
#define LIGHT_FRUSTUM_WIDTH 0.34641f // <- 계산해서 찾은 값

// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
// #define LIGHT_RADIUS_UV (LIGHT_WORLD_RADIUS / LIGHT_FRUSTUM_WIDTH)

float PCF_Filter(float2 uv, float zReceiverNdc, float filterRadiusUV, Texture2D shadowMap)
{
    float sum = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float2 offset = diskSamples64[i] * filterRadiusUV;
        sum += shadowMap.SampleCmpLevelZero(shadowCompareSampler, uv + offset, zReceiverNdc);
    }
    return sum / 64.0f;
}

// void Func(out float a) <- c++의 void Func(float& a) 처럼 출력값 저장 가능

void FindBlocker(out float avgBlockerDepthView, out float numBlockers, float2 uv,
                 float zReceiverView, Texture2D shadowMap, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float searchRadius = lightRadiusUV * (zReceiverView - NEAR_PLANE) / zReceiverView;

    float blockerSum = 0.0f;
    numBlockers = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        float shadowMapDepth = shadowMap.SampleLevel(shadowPointSampler, float2(uv + diskSamples64[i] * searchRadius), 0.0f).r;

        shadowMapDepth = N2V(shadowMapDepth, invProj);
        
        if (shadowMapDepth < zReceiverView)
        {
            blockerSum += shadowMapDepth;
            ++numBlockers;
        }
    }
    
    avgBlockerDepthView = blockerSum / numBlockers;
}

float PCSS(float2 uv, float zReceiverNdc, Texture2D shadowMap, matrix invProj, float lightRadiusWorld)
{
    float lightRadiusUV = lightRadiusWorld / LIGHT_FRUSTUM_WIDTH;
    float zReceiverView = N2V(zReceiverNdc, invProj);
    
    // STEP 1: blocker search.
    float avgBlockerDepthView = 0;
    float numBlockers = 0;

    FindBlocker(avgBlockerDepthView, numBlockers, uv, zReceiverView, shadowMap, invProj, lightRadiusWorld);

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
        return PCF_Filter(uv, zReceiverNdc, filterRadiusUV, shadowMap);
    }
}

float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld, Texture2D shadowMap)
{
    // Directional light.
    float3 lightVec = (light.type & LIGHT_DIRECTIONAL ?
                       -light.direction :
                       representativePoint - posWorld); // light.position - posWorld;

    float lightDist = length(lightVec);
    lightVec /= lightDist;

    // Spot light.
    float spotFator = (light.type & LIGHT_SPOT ?
                       pow(max(-dot(lightVec, light.direction), 0.0f), light.spotPower) :
                       1.0f);
        
    // Distance attenuation.
    float att = saturate((light.fallOffEnd - lightDist) / (light.fallOffEnd - light.fallOffStart));

    // Shadow map.
    float shadowFactor = 1.0f;

    if (light.type & LIGHT_SHADOW)
    {
        const float NEAR_Z = 0.01f; // 카메라 설정과 동일.
        
        // 1. Project posWorld to light screen  .  
        float4 lightScreen = mul(float4(posWorld, 1.0f), light.viewProj);
        lightScreen.xyz /= lightScreen.w;
        
        // 2. 카메라(광원)에서 볼 때의 텍스춰 좌표 계산.
        float2 lightTexcoord = float2(lightScreen.x, -lightScreen.y);
        lightTexcoord += 1.0f;
        lightTexcoord *= 0.5f;
        
        // 3. 쉐도우맵에서 값 가져오기.
        //float depth = shadowMap.Sample(shadowPointSampler, lightTexcoord).r;
        
        // 4. 가려져 있다면 그림자로 표시.
        //if (depth + 0.001 < lightScreen.z)
          //  shadowFactor = 0.0;
        
        uint width, height, numMips;
        shadowMap.GetDimensions(0, width, height, numMips);
        
        // float dx = 5.0 / (float) width;
        // shadowFactor = PCF_Filter(lightTexcoord.xy, lightScreen.z - 0.001, dx, shadowMap);
        
        float radiusScale = 0.5f; // 광원의 반지름을 키웠을 때 깨지는 것 방지.
        shadowFactor = PCSS(lightTexcoord, lightScreen.z - 0.001f, shadowMap, light.invProj, light.radius * radiusScale);
    }

    float3 radiance = light.radiance * spotFator * att * shadowFactor;

    return radiance;
}

[numthreads(8, 8, 1)]
void main(uint3 gID : SV_GroupID, uint3 gtID : SV_GroupThreadID, uint3 dtID : SV_DispatchThreadID)
{
    uint2 threadPos = dtID.xy;
    
    float4 albedo = g_AlbedoTex[threadPos];
    float3 normal = g_NormalTex[threadPos].xyz;
    float3 worldPos = g_WorldPositionTex[threadPos].xyz;
    float3 emission = g_EmissionTex[threadPos].rgb;
    float metallic = g_ExtraTex[threadPos].x;
    float roughness = g_ExtraTex[threadPos].y;
    float ao = g_ExtraTex[threadPos].z;
    float height = g_ExtraTex[threadPos].w;
    
    if (albedo.a - 0.5f < 0.0f) // Tree leaves. 투명한 부분의 픽셀은 그리지 않음.
    {
        g_OutputTexture[threadPos] = 0;
    }
    else
    {
        float3 pixelToEye = normalize(eyeWorld - worldPos);
        float3 ambientLighting = AmbientLightingByIBL(albedo.rgb, normal, pixelToEye, ao, metallic, roughness) * strengthIBL;
        float3 directLighting = float3(0.0f, 0.0f, 0.0f);
    
        [unroll]
        for (int i = 0; i < MAX_LIGHTS; ++i)
        {
            if (lights[i].type)
            {
                float3 L = lights[i].position - worldPos;
                float3 r = normalize(reflect(eyeWorld - worldPos, normal));
                float3 centerToRay = dot(L, r) * r - L;
                float3 representativePoint = L + centerToRay * clamp(lights[i].radius / length(centerToRay), 0.0f, 1.0f);
                representativePoint += worldPos;
                float3 lightVec = representativePoint - worldPos;

                //float3 lightVec = lights[i].position - worldPos;
                float lightDist = length(lightVec);
                lightVec /= lightDist;
                float3 halfway = normalize(pixelToEye + lightVec);
        
                float NdotI = max(0.0f, dot(normal, lightVec));
                float NdotH = max(0.0f, dot(normal, halfway));
                float NdotO = max(0.0f, dot(normal, pixelToEye));
        
                const float3 F_DIELECTRIC = 0.04; // 비금속(Dielectric) 재질의 F0
                float3 F0 = lerp(F_DIELECTRIC, albedo.rgb, metallic);
                float3 F = SchlickFresnel(F0, max(0.0f, dot(halfway, pixelToEye)));
                float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
                float3 diffuseBRDF = kd * albedo.rgb;

                // Sphere Normalization
                float alpha = roughness * roughness;
                float alphaPrime = saturate(alpha + lights[i].radius / (2.0f * lightDist));

                float D = NdfGGX(NdotH, roughness, alphaPrime);
                float3 G = SchlickGGX(NdotI, NdotO, roughness);
                float3 specularBRDF = (F * D * G) / max(1e-5, 4.0f * NdotI * NdotO);

                float3 radiance = LightRadiance(lights[i], representativePoint, worldPos, normal, shadowMaps[i]);
            
                // 오류 임시 수정 (radiance가 (0,0,0)일 경우, directLighting += ... 인데도 0 벡터가 되어버림.
                if (abs(dot(radiance, float3(1.0f, 1.0f, 1.0f))) > 1e-5)
                {
                    directLighting += (diffuseBRDF + specularBRDF) * radiance * NdotI;
                }
            }
        }
    
        g_OutputTexture[threadPos] += float4(ambientLighting + directLighting + emission, 1.0f);
    }
    
    g_OutputTexture[threadPos] = clamp(g_OutputTexture[threadPos], 0.0f, 1000.0f);
}
