#ifndef ATMOSPHERE_UTIL
#define ATMOSPHERE_UTIL

#include "Intersection.hlsli"

cbuffer AtmosphereConstants : register(b1)
{
    float3 g_ScatterRayleigh;
    float g_HDensityRayleigh;

    float g_ScatterMie;
    float g_AsymmetryMie;
    float g_AbsorbMie;
    float g_HDensityMie;

    float3 g_AbsorbOzone;
    float g_OzoneCenterHeight;

    float g_OzoneThickness;
    float g_PlanetRadius;
    float g_AtmosphereRadius;
}

float3 GetSigmaS(float h)
{
    float3 rayleigh = g_ScatterRayleigh * exp(-h / g_HDensityRayleigh);
    float mie = g_ScatterMie * exp(-h / g_HDensityMie);
    return rayleigh + mie;
}

float3 GetSigmaT(float h)
{
    float3 rayleigh = g_ScatterRayleigh * exp(-h / g_HDensityRayleigh);
    float mie = (g_ScatterMie + g_AbsorbMie) * exp(-h / g_HDensityMie);
    float3 ozone = g_AbsorbOzone * max(0.0f, 1.0f - 0.5f * abs(h - g_OzoneCenterHeight) / g_OzoneThickness);
    return rayleigh + mie + ozone;
}

void GetSigmaST(float h, out float3 sigmaS, out float3 sigmaT)
{
    float3 rayleigh = g_ScatterRayleigh * exp(-h / g_HDensityRayleigh);

    float mieDensity = exp(-h / g_HDensityMie);
    float mieS = g_ScatterMie * mieDensity;
    float mieT = (g_ScatterMie + g_AbsorbMie) * mieDensity;

    float3 ozone = g_AbsorbOzone * max(0.0f, 1.0f - 0.5f * abs(h - g_OzoneCenterHeight) / g_OzoneThickness);

    sigmaS = rayleigh + mieS;
    sigmaT = rayleigh + mieT + ozone;
}

float3 EvalPhaseFunction(float h, float u)
{
    float3 sRayleigh = g_ScatterRayleigh * exp(-h / g_HDensityRayleigh);
    float sMie = g_ScatterMie * exp(-h / g_HDensityMie);
    float3 s = sRayleigh + sMie;

    float g = g_AsymmetryMie;
    float g2 = g * g;
    float u2 = u * u;
    float pRayleigh = 3.0f / (16.0f * PI) * (1.0f + u2);

    float m = 1.0f + g2 - 2.0f * g * u;
    float pMie = 3.0f / (8.0f * PI) * (1.0f - g2) * (1.0f + u2) / ((2.0f + g2) * m * sqrt(m));

    float3 result;
    result.x = (s.x > 0.0f ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0.0f);
    result.y = (s.y > 0.0f ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0.0f);
    result.z = (s.z > 0.0f ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0.0f);
    return result;
}

float3 GetTransmittance(Texture2D<float3> T, SamplerState S, float h, float theta)
{
    float u = h / (g_AtmosphereRadius - g_PlanetRadius);
    float v = 0.5f + 0.5f * sin(theta);
    return T.SampleLevel(S, float2(u, v), 0.0f);
}

#endif