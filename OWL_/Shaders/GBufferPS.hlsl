#include "Common.hlsli"

// 메쉬 재질 텍스춰들 t0 부터 시작.
Texture2D g_AlbedoTex : register(t0);
Texture2D g_EmissiveTex : register(t1);
Texture2D g_NormalTex : register(t2);
Texture2D g_AOTex : register(t3);
Texture2D g_MetallicTex : register(t4);
Texture2D g_RoughnessTex : register(t5);
Texture2D g_HeightTexture : register(t6);

struct PixelShaderOutput
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
    float4 WorldPosition : SV_Target2;
    float4 Emission : SV_Target3;
    float4 Extra : SV_Target4;
};

float3 GetNormal(PixelShaderInput input)
{
    float3 normalWorld = normalize(input.WorldNormal);
    
    if (bUseNormalMap) // NormalWorld를 교체.
    {
        float3 normal = g_NormalTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).rgb;
        normal = 2.0f * normal - 1.0f; // 범위 조절 [-1.0, 1.0]

        // OpenGL 용 노멀맵일 경우에는 y 방향을 뒤집어줌.
        normal.y = (bInvertNormalMapY ? -normal.y : normal.y);
        
        float3 N = normalWorld;
        float3 T = normalize(input.WorldTangent - dot(input.WorldTangent, N) * N);
        float3 B = cross(N, T);
        
        // matrix는 float4x4, 여기서는 벡터 변환용이라서 3x3 사용.
        float3x3 TBN = float3x3(T, B, N);
        normalWorld = normalize(mul(normal, TBN));
    }
    
    return normalWorld;
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    
    float3 pixelToEye = normalize(g_EyeWorld - input.WorldPosition);
    float3 normalWorld = GetNormal(input);
    
    float4 albedo = (bUseAlbedoMap ? g_AlbedoTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias) * float4(g_AlbedoFactor, 1.0f) : float4(g_AlbedoFactor, 1.0f));
    // clip(albedo.a - 0.5f); // Tree leaves. 투명한 부분의 픽셀은 그리지 않음.
    float ao = (bUseAOMap ? g_AOTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).r : 1.0f);
    float metallic = (bUseMetallicMap ? g_MetallicTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).b * g_MetallicFactor : g_MetallicFactor);
    float roughness = (bUseRoughnessMap ? g_RoughnessTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).g * g_RoughnessFactor : g_RoughnessFactor);
    float3 emission = (bUseEmissiveMap ? g_EmissiveTex.SampleLevel(g_LinearWrapSampler, input.Texcoord, g_LODBias).rgb : g_EmissionFactor);
    float height = (bUseHeightMap ? g_HeightTexture.SampleLevel(g_LinearClampSampler, input.Texcoord, 0.0f).r : 0.0f);
    
    output.Albedo = albedo;
    output.Normal = float4(normalWorld, 1.0f);
    output.WorldPosition = float4(input.WorldPosition, input.ProjectedPosition.w);
    output.Emission = float4(emission, 1.0f);
    output.Extra = float4(metallic, roughness, ao, height);
    
    return output;
}