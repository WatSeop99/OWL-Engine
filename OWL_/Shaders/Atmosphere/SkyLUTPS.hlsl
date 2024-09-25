#include "Intersection.hlsli"
#include "AtmosphereUtil.hlsli"

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer SkyLUTConstants : register(b0)
{
    float3 g_AtmosEyePosition;
    int g_MarchStepCount;

    float3 g_SunDirection;
    bool g_bEnableMultiScattering;

    float3 g_SunIntensity;
}

Texture2D<float3> g_TransmittanceLUT : register(t0);
Texture2D<float3> g_MultiScatteringLUT : register(t1);

SamplerState g_MTSampler : register(s0);

void MarchStep(float phaseU, float3 ori, float3 dir, float thisT, float nextT, inout float3 sumSigmaT, inout float3 inScattering)
{
    float midT = 0.5f * (thisT + nextT);
    float3 posR = float3(0.0f, ori.y + g_PlanetRadius, 0.0f) + dir * midT;
    float h = length(posR) - g_PlanetRadius;

    float3 sigmaS, sigmaT;
    GetSigmaST(h, sigmaS, sigmaT);

    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 eyeTrans = exp(-sumSigmaT - 0.5f * deltaSumSigmaT);

    float sunTheta = PI / 2.0f - acos(dot(-g_SunDirection, normalize(posR)));

    if (!HasIntersectionWithSphere(posR, -g_SunDirection, g_PlanetRadius))
    {
        float3 rho = EvalPhaseFunction(h, phaseU);
        float3 sunTrans = GetTransmittance(g_TransmittanceLUT, g_MTSampler, h, sunTheta);

        inScattering += (nextT - thisT) * eyeTrans * sigmaS * rho * sunTrans;
    }

    if (g_bEnableMultiScattering)
    {
        float tx = h / (g_AtmosphereRadius - g_PlanetRadius);
        float ty = 0.5f + 0.5f * sin(sunTheta);
        float3 ms = g_MultiScatteringLUT.SampleLevel(g_MTSampler, float2(tx, ty), 0.0f);

        inScattering += (nextT - thisT) * eyeTrans * sigmaS * ms;
    }

    sumSigmaT += deltaSumSigmaT;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float phi = 2.0f * PI * input.texCoord.x;
    float vm = 2.0f * input.texCoord.y - 1.0f;
    float theta = sign(vm) * (PI / 2) * vm * vm;
    float sinTheta = sin(theta), cosTheta = cos(theta);

    float3 ori = g_AtmosEyePosition;
    float3 dir = float3(cos(phi) * cosTheta, sinTheta, sin(phi) * cosTheta);

    float2 planetOri = float2(0.0f, ori.y + g_PlanetRadius);
    float2 planetDir = float2(cosTheta, sinTheta);

    // find end point

    float endT = 0.0f;
    if (!FindClosestIntersectionWithCircle(planetOri, planetDir, g_PlanetRadius, endT))
    {
        FindClosestIntersectionWithCircle(planetOri, planetDir, g_AtmosphereRadius, endT);
    }

    // phase function input

    float phaseU = dot(g_SunDirection, -dir);

    // ray march

    float t = 0.0f;
    float3 inScatter = float3(0.0f, 0.0f, 0.0f);
    float3 sumSigmaT = float3(0.0f, 0.0f, 0.0f);

    float dt = (endT - t) / g_MarchStepCount;
    for (int i = 0; i < g_MarchStepCount; ++i)
    {
        float nextT = t + dt;
        MarchStep(phaseU, ori, dir, t, nextT, sumSigmaT, inScatter);
        t = nextT;
    }

    return float4(inScatter * g_SunIntensity, 1.0f);
}
