#include "Common.hlsli"
#include "ShadingUtil.hlsli"

struct SamplingPixelShaderInput
{
    float4 ScreenPosition : SV_POSITION;
    float2 Texcoord : TEXCOORD;
};
struct PixelShaderOutput
{
    float4 PixelColor : SV_Target0;
};

Texture2D<float4> g_AlbedoTex : register(t0);
Texture2D<float4> g_NormalTex : register(t1);
Texture2D<float4> g_WorldPositionTex : register(t2);
Texture2D<float4> g_EmissionTex : register(t3);
Texture2D<float4> g_ExtraTex : register(t4); // { metallic, roughness, ao, height }
Texture2D g_Shadow2DMap : register(t5);
TextureCube g_ShadowCubeMap : register(t6);
Texture2DArray g_CascadeShadowMaps : register(t7);

float3 LightRadiance(Light light, float3 representativePoint, float3 posWorld, float3 normalWorld)
{
    // Directional light.
    float3 lightVec = (light.Type & (LIGHT_DIRECTIONAL | LIGHT_SUN) ? -light.Direction : representativePoint - posWorld); // light.position - posWorld;
    float lightDist = length(lightVec);
    lightVec /= lightDist;

    // Spot light.
    float spotFator = (light.Type & LIGHT_SPOT ? pow(max(-dot(lightVec, light.Direction), 0.0f), light.SpotPower) : 1.0f);
        
    // Distance attenuation.
    float att = saturate((light.FallOffEnd - lightDist) / (light.FallOffEnd - light.FallOffStart));

    // Shadow map.
    float shadowFactor = 1.0f;

    if (light.Type & LIGHT_SHADOW)
    {
        float4 lightScreen = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float3 lightTexcoord = float3(0.0f, 0.0f, 0.0f);
        float radiusScale = 0.5f; // 광원의 반지름을 키웠을 때 깨지는 것 방지.
        
        switch (light.Type & (LIGHT_DIRECTIONAL | LIGHT_POINT | LIGHT_SPOT | LIGHT_SUN))
        {
            case LIGHT_SUN:
                {
                    int index = -1;
                    
                    for (int i = 0; i < 4; ++i)
                    {
                        lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[i]);
                        lightScreen.xyz /= lightScreen.w;
                        if (lightScreen.z > 1.0f)
                        {
                            continue;
                        }
                    
                        lightTexcoord.xy = float2(lightScreen.x, -lightScreen.y);
                        lightTexcoord.xy += 1.0f;
                        lightTexcoord.xy *= 0.5f;
                    
                        float depth = g_CascadeShadowMaps.SampleLevel(g_ShadowPointSampler, float3(lightTexcoord.xy, i), 0.0f);
                        if (depth <= lightScreen.z - 0.005f || depth >= lightScreen.z + 0.005f)
                        {
                            index = i;
                            break;
                        }
                    }
                    
                    if (index != -1)
                    {
                        shadowFactor = PCSSForDirectionalLight(g_CascadeShadowMaps, g_ShadowPointSampler, g_ShadowCompareSampler, index, lightTexcoord.xy, lightScreen.z - 0.001f, light.InverseProjections[index], light.Radius * radiusScale);
                    }
                }
                break;
            
            case LIGHT_POINT:
                {
                    const float3 VIEW_DIRs[6] =
                    {
                        float3(1.0f, 0.0f, 0.0f), // right
			            float3(-1.0f, 0.0f, 0.0f), // left
			            float3(0.0f, 1.0f, 0.0f), // up
			            float3(0.0f, -1.0f, 0.0f), // down
			            float3(0.0f, 0.0f, 1.0f), // front
			            float3(0.0f, 0.0f, -1.0f) // back 
                    };
                    int index = 0;
                    float maxDotProduct = -2.0f;
                    float3 lightToPos = normalize(posWorld - light.Position);
                    
                    for (int i = 0; i < 6; ++i)
                    {
                        float curDot = dot(lightToPos, VIEW_DIRs[i]);
                        if (maxDotProduct < curDot)
                        {
                            maxDotProduct = curDot;
                            index = i;
                        }
                    }
        
                    lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[index]);
                    lightScreen.xyz /= lightScreen.w;
        
                    lightTexcoord = lightToPos;
        
                    shadowFactor = PCSSForPointLight(g_ShadowCubeMap, g_ShadowPointSampler, g_ShadowCompareSampler, lightTexcoord, lightScreen.z - 0.001f, light.InverseProjections[0], light.Radius * radiusScale);
                }
                break;
            
            case LIGHT_DIRECTIONAL:
            case LIGHT_SPOT:
                {
                    // Project posWorld to light screen.  
                    lightScreen = mul(float4(posWorld, 1.0f), light.ViewProjection[0]);
                    lightScreen.xyz /= lightScreen.w;
        
                    // 카메라(광원)에서 볼 때의 텍스춰 좌표 계산. ([-1, 1], [-1, 1]) ==> ([0, 1], [0, 1])
                    lightTexcoord.xy = float2(lightScreen.x, -lightScreen.y);
                    lightTexcoord.xy += 1.0f;
                    lightTexcoord.xy *= 0.5f;
       
                    shadowFactor = PCSSForSpotLight(g_Shadow2DMap, g_ShadowPointSampler, g_ShadowCompareSampler, lightTexcoord.xy, lightScreen.z - 0.001f, light.InverseProjections[0], light.Radius * radiusScale);
                }
                break;
            
            default:
                break;
        }
    }

    float3 radiance = light.Radiance * spotFator * att * shadowFactor;
    return radiance;
}

PixelShaderOutput main(SamplingPixelShaderInput input)
{
    PixelShaderOutput output;
    output.PixelColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float4 albedo = g_AlbedoTex.Sample(g_LinearWrapSampler, input.Texcoord);
    float3 normal = g_NormalTex.Sample(g_LinearWrapSampler, input.Texcoord).xyz;
    float3 worldPos = g_WorldPositionTex.Sample(g_LinearWrapSampler, input.Texcoord).xyz;
    float3 emission = g_EmissionTex.Sample(g_LinearWrapSampler, input.Texcoord).rgb;
    float metallic = g_ExtraTex.Sample(g_LinearWrapSampler, input.Texcoord).x;
    float roughness = g_ExtraTex.Sample(g_LinearWrapSampler, input.Texcoord).y;
    float ao = g_ExtraTex.Sample(g_LinearWrapSampler, input.Texcoord).z;
    float height = g_ExtraTex.Sample(g_LinearWrapSampler, input.Texcoord).w;
    if (albedo.a - 0.5f == 0.0f) // Tree leaves. 투명한 부분의 픽셀은 그리지 않음.
    {
        return output;
    }
    
    float3 pixelToEye = normalize(g_EyeWorld - worldPos);
    float3 ambientLighting = AmbientLightingByIBL(g_IrradianceIBLTex, g_SpecularIBLTex, g_BRDFTex, g_LinearClampSampler, g_LinearWrapSampler, albedo.rgb, normal, pixelToEye, ao, metallic, roughness) * g_StrengthIBL;
    float3 directLighting = float3(0.0f, 0.0f, 0.0f);
    
    if (lights.Type)
    {
        float3 L = lights.Position - worldPos;
        float3 r = normalize(reflect(g_EyeWorld - worldPos, normal));
        float3 centerToRay = dot(L, r) * r - L;
        float3 representativePoint = L + centerToRay * clamp(lights.Radius / length(centerToRay), 0.0f, 1.0f);
        representativePoint += worldPos;
        float3 lightVec = representativePoint - worldPos;

            //float3 lightVec = lights[i].position - worldPos;
        float lightDist = length(lightVec);
        lightVec /= lightDist;
        float3 halfway = normalize(pixelToEye + lightVec);
        
        float NdotI = max(0.0f, dot(normal, lightVec));
        float NdotH = max(0.0f, dot(normal, halfway));
        float NdotO = max(0.0f, dot(normal, pixelToEye));
        
        float3 F0 = lerp(F_DIELECTRIC, albedo.rgb, metallic);
        float3 F = SchlickFresnel(F0, max(0.0f, dot(halfway, pixelToEye)));
        float3 kd = lerp(float3(1.0f, 1.0f, 1.0f) - F, float3(0.0f, 0.0f, 0.0f), metallic);
        float3 diffuseBRDF = kd * albedo.rgb;

            // Sphere Normalization
        float alpha = roughness * roughness;
        float alphaPrime = saturate(alpha + lights.Radius / (2.0f * lightDist));

        float D = NdfGGX(NdotH, roughness, alphaPrime);
        float3 G = SchlickGGX(NdotI, NdotO, roughness);
        float3 specularBRDF = (F * D * G) / max(1e-5, 4.0f * NdotI * NdotO);

        float3 radiance = float3(0.0f, 0.0f, 0.0f);
        radiance = LightRadiance(lights, representativePoint, worldPos, normal);
            
            // 오류 임시 수정 (radiance가 (0,0,0)일 경우, directLighting += ... 인데도 0 벡터가 되어버림.
        if (abs(dot(radiance, float3(1.0f, 1.0f, 1.0f))) > 1e-5)
        {
            directLighting += (diffuseBRDF + specularBRDF) * radiance * NdotI;
        }
    }
    
    output.PixelColor = float4(ambientLighting + directLighting + emission, 1.0f);
    output.PixelColor = clamp(output.PixelColor, 0.0f, 1000.0f);
    return output;
}
