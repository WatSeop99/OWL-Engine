#include "Intersection.hlsli"
#include "AtmosphereUtil.hlsli"

static const int THREAD_GROUP_SIZE_X = 16;
static const int THREAD_GROUP_SIZE_Y = 16;

cbuffer MultiScatterConstants : register(b0)
{
    float3 g_TerrainAlbedo;
    int g_DirSampleCount;

    float3 g_SunIntensity;
    int g_RayMarchStepCount;
}

Texture2D<float3> g_TransmittanceLUT : register(t0);
StructuredBuffer<float2> g_RawDirSamples : register(t1);

SamplerState g_TransmittanceSampler : register(s0);

RWTexture2D<float4> g_MultiScatteringLUT : register(u0);

float3 UniformOnUnitSphere(float u1, float u2)
{
    float z = 1.0f - 2.0f * u1;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2.0f * PI * u2;
    return float3(r * cos(phi), r * sin(phi), z);
}

void Integrate(float3 worldOri, float3 worldDir, float sunTheta, float3 toSunDir, out float3 innerL2, out float3 innerF)
{
    float u = dot(worldDir, toSunDir);

    float endT = 0.0f;
    bool bGroundInct = FindClosestIntersectionWithSphere(worldOri, worldDir, g_PlanetRadius, endT);
    if (!bGroundInct)
    {
        FindClosestIntersectionWithSphere(worldOri, worldDir, g_AtmosphereRadius, endT);
    }

    float dt = endT / g_RayMarchStepCount;
    float halfDt = 0.5f * dt;
    float t = 0.0f;

    float3 sumSigmaT = float3(0.0f, 0.0f, 0.0f);

    float3 sumL2 = float3(0.0f, 0.0f, 0.0f);
    float3 sumF = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < g_RayMarchStepCount; ++i)
    {
        float midT = t + halfDt;
        t += dt;

        float3 worldPos = worldOri + midT * worldDir;
        float h = length(worldPos) - g_PlanetRadius;

        float3 sigmaS, sigmaT;
        GetSigmaST(h, sigmaS, sigmaT);

        float3 deltaSumSigmaT = dt * sigmaT;
        float3 transmittance = exp(-sumSigmaT - 0.5f * deltaSumSigmaT);

        if (!HasIntersectionWithSphere(worldPos, toSunDir, g_PlanetRadius))
        {
            float3 rho = EvalPhaseFunction(h, u);
            float3 sunTransmittance = GetTransmittance(g_TransmittanceLUT, g_TransmittanceSampler, h, sunTheta);

            sumL2 += dt * transmittance * sunTransmittance * sigmaS * rho * g_SunIntensity;
        }

        sumF += dt * transmittance * sigmaS;
        sumSigmaT += deltaSumSigmaT;
    }

    if (bGroundInct)
    {
        float3 transmittance = exp(-sumSigmaT);
        float3 sunTransmittance = GetTransmittance(g_TransmittanceLUT, g_TransmittanceSampler, 0, sunTheta);
        sumL2 += transmittance * sunTransmittance * max(0, toSunDir.y) * g_SunIntensity * (g_TerrainAlbedo / PI);
    }

    innerL2 = sumL2;
    innerF = sumF;
}

float3 ComputeM(float h, float sunTheta)
{
    float3 worldOri = float3(0.0f, h + g_PlanetRadius, 0.0f);
    float3 toSunDir = float3(cos(sunTheta), sin(sunTheta), 0.0f);

    float3 sumL2 = float3(0.0f, 0.0f, 0.0f);
    float3 sumF = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < g_DirSampleCount; ++i)
    {
        float2 rawSample = g_RawDirSamples[i];
        float3 worldDir = UniformOnUnitSphere(rawSample.x, rawSample.y);

        float3 innerL2, innerF;
        Integrate(worldOri, worldDir, sunTheta, toSunDir, innerL2, innerF);

        // phase function is canceled by pdf

        sumL2 += innerL2;
        sumF += innerF;
    }

    float3 l2 = sumL2 / g_DirSampleCount;
    float3 f = sumF / g_DirSampleCount;
    return l2 / (1.0f - f);
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void main(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    g_MultiScatteringLUT.GetDimensions(width, height);
    if (threadIdx.x >= width || threadIdx.y >= height)
    {
        return;
    }
    float sinSunTheta = lerp(-1.0f, 1.0f, (threadIdx.y + 0.5f) / height);
    float sunTheta = asin(sinSunTheta);

    float h = lerp(0.0f, g_AtmosphereRadius - g_PlanetRadius, (threadIdx.x + 0.5f) / width);

    g_MultiScatteringLUT[threadIdx.xy] = float4(ComputeM(h, sunTheta), 1.0f);
}
