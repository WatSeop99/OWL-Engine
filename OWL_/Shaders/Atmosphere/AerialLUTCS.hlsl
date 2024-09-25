#include "Intersection.hlsli"
#include "AtmosphereUtil.hlsli"

static const int THREAD_GROUP_SIZE_X = 16;
static const int THREAD_GROUP_SIZE_Y = 16;

cbuffer AerialLUTConstants : register(b0)
{
    float3 g_SunDirection;
    float g_SunTheta;

    float3 g_FrustumA;
    float g_MaxDistance;
    float3 g_FrustumB;
    int g_PerSliceMarchStepCount;
    float3 g_FrustumC;
    int g_EnableMultiScattering;
    float3 g_FrustumD;
    float g_AtmosEyeHeight;

    float3 g_EyePosition;
    int g_EnableShadow;
    matrix g_ShadowViewProj;
    float g_WorldScale;
}

Texture2D<float3> g_MultiScattering : register(t0);
Texture2D<float3> g_Transmittance : register(t1);
Texture2D<float> g_ShadowMap : register(t2);

SamplerState g_MTSampler : register(s0);
SamplerState g_ShadowSampler : register(s1);

RWTexture3D<float4> g_AerialPerspectiveLUT : register(u0);

float RelativeLuminance(float3 c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void main(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height, depth;
    g_AerialPerspectiveLUT.GetDimensions(width, height, depth);
    if (threadIdx.x >= width || threadIdx.y >= height)
    {
        return;
    }

    float xf = (threadIdx.x + 0.5f) / width;
    float yf = (threadIdx.y + 0.5f) / height;

    float3 ori = float3(0.0f, g_AtmosEyeHeight, 0.0f);
    float3 dir = normalize(lerp(lerp(g_FrustumA, g_FrustumB, xf), lerp(g_FrustumC, g_FrustumD, xf), yf));

    float u = dot(g_SunDirection, -dir);

    float maxT = 0.0f;
    if (!FindClosestIntersectionWithSphere(ori + float3(0.0f, g_PlanetRadius, 0.0f), dir, g_PlanetRadius, maxT))
    {
        FindClosestIntersectionWithSphere(ori + float3(0.0f, g_PlanetRadius, 0.0f), dir, g_AtmosphereRadius, maxT);
    }

    float sliceDepth = g_MaxDistance / depth;
    float halfSliceDepth = 0.5f * sliceDepth;
    float tBeg = 0.0f;
    float tEnd = min(halfSliceDepth, maxT);

    float3 sumSigmaT = float3(0.0f, 0.0f, 0.0f);
    float3 inScatter = float3(0.0f, 0.0f, 0.0f);

    float rand = frac(sin(dot(float2(xf, yf), float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f);

    for (int z = 0; z < depth; ++z)
    {
        float dt = (tEnd - tBeg) / g_PerSliceMarchStepCount;
        float t = tBeg;

        for (int i = 0; i < g_PerSliceMarchStepCount; ++i)
        {
            float nextT = t + dt;

            float midT = lerp(t, nextT, rand);
            float3 posR = float3(0, ori.y + g_PlanetRadius, 0) + dir * midT;
            float h = length(posR) - g_PlanetRadius;

            //float sunTheta = PI / 2 - acos(dot(-g_SunDirection, normalize(posR)));

            float3 sigmaS, sigmaT;
            GetSigmaST(h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 eyeTrans = exp(-sumSigmaT - 0.5f * deltaSumSigmaT);

            if (!HasIntersectionWithSphere(posR, -g_SunDirection, g_PlanetRadius))
            {
                float3 shadowPos = g_EyePosition + dir * midT / g_WorldScale;
                float4 shadowClip = mul(float4(shadowPos, 1.0f), g_ShadowViewProj);
                float2 shadowNDC = shadowClip.xy / shadowClip.w;
                float2 shadowUV = 0.5f + float2(0.5f, -0.5f) * shadowNDC;

                bool bInShadow = g_EnableShadow;
                if (g_EnableShadow && all(saturate(shadowUV) == shadowUV))
                {
                    float rayZ = shadowClip.z;
                    float smZ = g_ShadowMap.SampleLevel(g_ShadowSampler, shadowUV, 0.0f);
                    bInShadow = rayZ >= smZ;
                }

                if (!bInShadow)
                {
                    float3 rho = EvalPhaseFunction(h, u);
                    float3 sunTrans = GetTransmittance(g_Transmittance, g_MTSampler, h, g_SunTheta);
                    inScatter += dt * eyeTrans * sigmaS * rho * sunTrans;
                }
            }

            if (g_EnableMultiScattering)
            {
                float tx = h / (g_AtmosphereRadius - g_PlanetRadius);
                float ty = 0.5f + 0.5f * sin(g_SunTheta);
                float3 ms = g_MultiScattering.SampleLevel(g_MTSampler, float2(tx, ty), 0.0f);
                inScatter += dt * eyeTrans * sigmaS * ms;
            }

            sumSigmaT += deltaSumSigmaT;
            t = nextT;
        }

        float transmittance = RelativeLuminance(exp(-sumSigmaT));
        g_AerialPerspectiveLUT[int3(threadIdx.xy, z)] = float4(inScatter, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}
