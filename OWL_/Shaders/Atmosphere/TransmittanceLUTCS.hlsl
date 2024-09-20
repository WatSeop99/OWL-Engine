#include "Intersection.hlsli"
#include "AtmosphereUtil.hlsli"

static const int THREAD_GROUP_SIZE_X = 16;
static const int THREAD_GROUP_SIZE_Y = 16;
static const int STEP_COUNT = 1000;

RWTexture2D<float4> g_Transmittance : register(u0);

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void main(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    g_Transmittance.GetDimensions(width, height);
    if (threadIdx.x >= width || threadIdx.y >= height)
    {
        return;
    }
    float theta = asin(lerp(-1.0f, 1.0f, (threadIdx.y + 0.5f) / height));
    float h = lerp(0.0f, g_AtmosphereRadius - g_PlanetRadius, (threadIdx.x + 0.5f) / width);

    float2 o = float2(0, g_PlanetRadius + h);
    float2 d = float2(cos(theta), sin(theta));
    
    float t = 0.0f;
    if (!FindClosestIntersectionWithCircle(o, d, g_PlanetRadius, t))
    {
        FindClosestIntersectionWithCircle(o, d, g_AtmosphereRadius, t);
    }
    float2 end = o + t * d;

    float3 sum = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < STEP_COUNT; ++i)
    {
        float2 pi = lerp(o, end, (i + 0.5f) / STEP_COUNT);
        float hi = length(pi) - g_PlanetRadius;
        float3 sigma = GetSigmaT(hi);
        sum += sigma;
    }

    float3 result = exp(-sum * (t / STEP_COUNT));
    g_Transmittance[threadIdx.xy] = float4(result, 1.0f);
}
