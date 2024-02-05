#include "common.hlsli"

// Very fast procedural ocean 
// https://www.shadertoy.com/view/MdXyzX

// afl_ext 2017-2023
// Now with 2023 refresh

// Use your mouse to move the camera around! Press the Left Mouse Button on the image to look around!

#define DRAG_MULT 0.28f // changes how much waves pull on the water
#define WATER_DEPTH 1.0f // how deep is the water
#define CAMERA_HEIGHT 1.5f // how high the camera should be
#define ITERATIONS_RAYMARCH 12 // waves iterations of raymarching
#define ITERATIONS_NORMAL 40 // waves iterations when calculating normals

// #define NormalizedMouse (iMouse.xy / iResolution.xy) // normalize mouse coords

// Calculates wave value and its derivative, 
// for the wave direction, position in space, wave frequency and time
float2 WaveDX(float2 position, float2 direction, float frequency, float timeshift)
{
    float x = dot(direction, position) * frequency + timeshift;
    float wave = exp(sin(x) - 1.0f);
    float dx = wave * cos(x);
    return float2(wave, -dx);
}

// Calculates waves by summing octaves of various waves with various parameters
float GetWaves(float2 position, int iterations)
{
    float iter = 0.0f; // this will help generating well distributed wave directions
    float frequency = 1.3f; // frequency of the wave, this will change every iteration
    float timeMultiplier = 1.0f; // time multiplier for the wave, this will change every iteration
    float weight = 0.5f; // weight in final sum for the wave, this will change every iteration
    float sumOfValues = 0.0f; // will store final sum of values
    float sumOfWeights = 0.0f; // will store final sum of weights
    for (int i = 0; i < iterations; ++i)
    {
        // generate some wave direction that looks kind of random
        float2 p = float2(sin(iter), cos(iter));
        
        // calculate wave data
        float2 res = WaveDX(position, p, frequency, globalTime * timeMultiplier);

        // shift position around according to wave drag and derivative of the wave
        position += p * res.y * weight * DRAG_MULT;

        // add the results to sums
        sumOfValues += res.x * weight;
        sumOfWeights += weight;

        // modify next octave parameters
        weight *= 0.82f;
        frequency *= 1.18f;
        timeMultiplier *= 1.07f;

        // add some kind of random value to make next wave look random too
        iter += 1232.399963f;
    }
    
    // calculate and return
    return sumOfValues / sumOfWeights;
}

// Raymarches the ray from top water layer boundary to low water layer boundary
float Raymarchwater(float3 camera, float3 start, float3 end, float depth)
{
    float3 pos = start;
    float3 dir = normalize(end - start);
    for (int i = 0; i < 32; ++i)
    {
        // the height is from 0 to -depth
        float height = GetWaves(pos.xz, ITERATIONS_RAYMARCH) * depth - depth;
        
        // if the waves height almost nearly matches the ray height, assume its a hit and return the hit distance
        if (height + 0.01f > pos.y)
        {
            return distance(pos, camera);
        }
        
        // iterate forwards according to the height mismatch
        pos += dir * (pos.y - height);
    }
    
    // if hit was not registered, just assume hit the top layer, 
    // this makes the raymarching faster and looks better at higher distances
    return distance(start, camera);
}

// Calculate normal at point by calculating the height at the pos and 2 additional points very close to pos
float3 Normal(float2 pos, float e, float depth)
{
    float2 ex = float2(e, 0.0f);
    float H = GetWaves(pos.xy, ITERATIONS_NORMAL) * depth;
    float3 a = float3(pos.x, H, pos.y);
    return normalize(cross(a - float3(pos.x - e, GetWaves(pos.xy - ex.xy, ITERATIONS_NORMAL) * depth, pos.y),
                           a - float3(pos.x, GetWaves(pos.xy + ex.yx, ITERATIONS_NORMAL) * depth, pos.y + e)));
}

// Ray-Plane intersection checker
float IntersectPlane(float3 origin, float3 direction, float3 pos, float3 normal)
{
    return clamp(dot(pos - origin, normal) / dot(direction, normal), -1.0f, 9991999.0f);
}

// Some very barebones but fast atmosphere approximation
float3 ExtraCheapAtmosphere(float3 raydir, float3 sundir)
{
    sundir.y = max(sundir.y, -0.07f);
    
    float specialTrick = 1.0f / (raydir.y * 1.0f + 0.1f);
    float specialTrick2 = 1.0f / (sundir.y * 11.0f + 1.0f);
    float raysundt = pow(abs(dot(sundir, raydir)), 2.0f);
    float sundt = pow(abs(max(0.0f, dot(sundir, raydir))), 8.0f);
    float mymie = sundt * specialTrick * 0.2f;
    float3 suncolor = lerp(float3(1.0f, 1.0f, 1.0f), max(float3(1.0f, 1.0f, 1.0f), float3(1.0f, 1.0f, 1.0f) - float3(5.5f, 13.0f, 22.4f) / 22.4f), specialTrick2);
    float3 bluesky = float3(5.5f, 13.0f, 22.4f) / 22.4f * suncolor;
    float3 bluesky2 = max(float3(1.0f, 1.0f, 1.0f), bluesky - float3(5.5f, 13.0f, 22.4f) * 0.002f * (specialTrick + -6.0f * sundir.y * sundir.y));
    bluesky2 *= specialTrick * (0.24f + raysundt * 0.24f);
    
    return bluesky2 * (1.0f + 1.0f * pow(1.0 - raydir.y, 3.0f)) + mymie * suncolor;
}

// Calculate where the sun should be, it will be moving around the sky
float3 GetSunDirection()
{
    return normalize(float3(sin(globalTime * 0.1f), 1.0f, cos(globalTime * 0.1f)));
}

// Get atmosphere color for given direction
float3 GetAtmosphere(float3 dir)
{
    return ExtraCheapAtmosphere(dir, GetSunDirection()) * 0.5f;
}

// Get sun color for given direction
float GetSun(float3 dir)
{
    return pow(max(0.0f, dot(dir, GetSunDirection())), 720.0f) * 210.0f;
}

// Main
float4 main(PixelShaderInput input) : SV_Target0
{
    float3 ray = -normalize(eyeWorld - input.posWorld);

    // now ray.y must be negative, water must be hit
    // define water planes
    float3 waterPlaneHigh = float3(0.0f, 0.0f, 0.0f);
    float3 waterPlaneLow = float3(0.0f, -WATER_DEPTH, 0.0f);

    // define ray origin, moving around
    //float3 origin = float3(globalTime, CAMERA_HEIGHT, globalTime);
   // float3 origin = float3(0, CAMERA_HEIGHT, 0);
    float3 origin = eyeWorld;

    // calculate intersections and reconstruct positions
    float highPlaneHit = IntersectPlane(origin, ray, waterPlaneHigh, float3(0.0f, 1.0f, 0.0f));
    float lowPlaneHit = IntersectPlane(origin, ray, waterPlaneLow, float3(0.0f, 1.0f, 0.0f));
    float3 highHitPos = origin + ray * highPlaneHit;
    float3 lowHitPos = origin + ray * lowPlaneHit;

    // raymatch water and reconstruct the hit pos
    float dist = Raymarchwater(origin, highHitPos, lowHitPos, WATER_DEPTH);
    float3 waterHitPos = origin + ray * dist;

    // calculate normal at the hit position
    float3 N = Normal(waterHitPos.xz, 0.0001f, WATER_DEPTH);

    // smooth the normal with distance to avoid disturbing high frequency noise
    N = lerp(N, float3(0.0f, 1.0f, 0.0f), 0.8f * min(1.0f, sqrt(dist * 0.01f) * 1.1f));

    // calculate fresnel coefficient
    float fresnel = (0.04f + (1.0f - 0.04f) * (pow(1.0f - max(0.0f, dot(-N, ray)), 5.0f)));

    // reflect the ray and make sure it bounces up
    float3 R = normalize(reflect(ray, N));
    R.y = abs(R.y);
  
    // calculate the reflection and approximate subsurface scattering
    float3 reflection = GetAtmosphere(R) + GetSun(R);
    float3 scattering = float3(0.0293f, 0.0698f, 0.1717f) * (0.2f + (waterHitPos.y + WATER_DEPTH) / WATER_DEPTH);

    // return the combined result
    float3 C = fresnel * reflection + (1.0f - fresnel) * scattering;
    
    return float4(C, 0.8f);
}