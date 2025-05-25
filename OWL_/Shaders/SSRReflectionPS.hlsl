#include "CommonHeader/GlobalResource.hlsli"

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 Tex : TEX;
};

cbuffer SSRConstants : register(b1)
{
    float g_SSRRayStep;
    float g_SSRRayHitThreshold;
    float dummy2[2];
}

Texture2D<float4> g_NormalTex : register(t0);
Texture2D<float4> g_ExtraTex : register(t1); // { metallic, roughness, ao, height }
Texture2D<float4> g_SceneTex : register(t2);
Texture2D<float> g_DepthTex : register(t3);

static const int SSR_MAX_STEPS = 16;
static const int SSR_BINARY_SEARCH_STEPS = 16;

float3 GetViewSpacePosition(in float2 texcoord, in float depth)
{
    float4 clipSpaceLocation;
    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
    clipSpaceLocation.y *= -1;
    clipSpaceLocation.z = depth;
    clipSpaceLocation.w = 1.0f;
    float4 homogenousLocation = mul(clipSpaceLocation, g_InverseProjection);

    return homogenousLocation.xyz / homogenousLocation.w;
}

float4 SSRBinarySearch(in float3 dir, inout float3 hitCoord)
{
    float depth;
    for (int i = 0; i < SSR_BINARY_SEARCH_STEPS; ++i)
    {
        float4 projectedCoord = mul(float4(hitCoord, 1.0f), g_Projection);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

        depth = g_DepthTex.SampleLevel(g_PointClampSampler, projectedCoord.xy, 0);
        float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
        float depthDifference = hitCoord.z - viewSpacePosition.z;

        if (depthDifference <= 0.0f)
        {
            hitCoord += dir;
        }
        dir *= 0.5f;
        hitCoord -= dir;
    }

    float4 projectedCoord = mul(float4(hitCoord, 1.0f), g_Projection);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    
    depth = g_DepthTex.SampleLevel(g_PointClampSampler, projectedCoord.xy, 0);
    float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
    float depthDifference = hitCoord.z - viewSpacePosition.z;

    return float4(projectedCoord.xy, depth, abs(depthDifference) < g_SSRRayHitThreshold ? 1.0f : 0.0f);
}

float4 SSRRayMarch(in float3 dir, inout float3 hitCoord)
{
    float depth;
    for (int i = 0; i < SSR_MAX_STEPS; ++i)
    {
        hitCoord += dir;

        float4 projectedCoord = mul(float4(hitCoord, 1.0f), g_Projection);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

        depth = g_DepthTex.SampleLevel(g_PointClampSampler, projectedCoord.xy, 0.0f);
        float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
        float depthDifference = hitCoord.z - viewSpacePosition.z;

		[branch]
        if (depthDifference > 0.0f)
        {
            return SSRBinarySearch(dir, hitCoord);
        }

        dir *= g_SSRRayStep;
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}


//bool IsInsideScreen(in float2 vCoord)
//{
//    return !(vCoord.x < 0.0f || vCoord.x > 1.0f || vCoord.y < 0.0f || vCoord.y > 1.0f);
//}

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = g_NormalTex.Sample(g_LinearPointBorderSampler, input.Tex).rgb;
    float4 sceneColor = g_SceneTex.SampleLevel(g_LinearClampSampler, input.Tex, 0.0f);
    
    float metallic = g_ExtraTex.Sample(g_LinearPointBorderSampler, input.Tex).r;
    if (metallic < 0.01f)
    {
        return sceneColor;
    }
    
    normal = 2.0f * normal - 1.0f;

    float depth = g_DepthTex.Sample(g_LinearClampSampler, input.Tex);
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 reflectDirection = normalize(reflect(viewSpacePosition, normal));

    float3 hitPosition = viewSpacePosition;
    float4 coords = SSRRayMarch(reflectDirection, hitPosition);

    float2 coordsEdgeFactor = float2(1.0f, 1.0f) - pow(saturate(abs(coords.xy - float2(0.5f, 0.5f)) * 2.0f), 8.0f);
    float screenEdgeFactor = saturate(min(coordsEdgeFactor.x, coordsEdgeFactor.y));
    float reflectionIntensity = saturate(screenEdgeFactor * saturate(reflectDirection.z) * (coords.w));

    float3 reflectionColor = reflectionIntensity * g_SceneTex.SampleLevel(g_LinearClampSampler, coords.xy, 0.0f).rgb;
    return sceneColor + metallic * max(0.0f, float4(reflectionColor, 1.0f));
}
