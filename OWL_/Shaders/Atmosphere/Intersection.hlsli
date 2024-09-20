#ifndef INTERSECION_HLSLI
#define INTERSECION_HLSLI

static const float PI = 3.14159265f;

bool HasIntersectionWithCircle(float2 o, float2 d, float R)
{
    float A = dot(d, d);
    float B = 2.0f * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4.0f * A * C;
    return (delta >= 0.0f && (C <= 0.0f || B <= 0.0f));
}

bool HasIntersectionWithSphere(float3 o, float3 d, float R)
{
    float A = dot(d, d);
    float B = 2.0f * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4.0f * A * C;
    return (delta >= 0.0f && (C <= 0.0f || B <= 0.0f));
}

bool FindClosestIntersectionWithCircle(float2 o, float2 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2.0f * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4.0f * A * C;
    if (delta < 0.0f)
    {
        return false;
    }
    t = (-B + (C <= 0.0f ? sqrt(delta) : -sqrt(delta))) / (2.0f * A);
    return (C <= 0.0f || B <= 0.0f);
}

bool FindClosestIntersectionWithSphere(float3 o, float3 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2.0f * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4.0f * A * C;
    if (delta < 0.0f)
    {
        return false;
    }
    t = (-B + (C <= 0.0f ? sqrt(delta) : -sqrt(delta))) / (2.0f * A);
    return (C <= 0.0f || B <= 0.0f);
}

#endif