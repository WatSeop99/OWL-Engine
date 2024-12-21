#ifndef __TILEABLE_NOISE_HLSLI__
#define __TILEABLE_NOISE_HLSLI__

#include "Hash.hlsli"

// https://www.shadertoy.com/view/3dVXDc

//// Hash by David_Hoskins
//#define UI0 1597334673U
//#define UI1 3812015801U
//#define UI2 uint2(UI0, UI1)
//#define UI3 uint3(UI0, UI1, 2798796415U)
//#define UIF (1.0f / (float)0xFFFFFFFFU)

//// if you need another has33 check link: https://www.shadertoy.com/view/XlXcW4
//float3 Hash33(float3 p)
//{
//    uint3 q = uint3(int3(p)) * UI3;
//    q = (q.x ^ q.y ^ q.z) * UI3;
//    return -1.0f + 2.0f * float3(q) * UIF;
//}

//float Hash21(uint2 p)
//{
//    //p *= uint2(73333, 7777);
//    //p ^= (uint2(3333777777) >> (p >> 28));
//    //float n = p.x * p.y;
//    //return n * UIF;
//    uint2 q = 1103515245U * ((p >> 1U) ^ (p.yx));
//    uint n = 1103515245U * ((q.x) ^ (q.y >> 3U));
//    return float(n) * UIF;
//}

float Remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

// Gradient noise by iq (modified to be tileable)
float GradientNoise(float3 x, float freq)
{
    // grid
    float3 p = floor(x);
    float3 w = frac(x);
    
    // quintic interpolant
    float3 u = w * w * w * (w * (w * 6.0f - 15.0f) + 10.0f);
    
    // gradients
    float3 ga = Hash33(fmod(p + float3(0.0f, 0.0f, 0.0f), freq));
    float3 gb = Hash33(fmod(p + float3(1.0f, 0.0f, 0.0f), freq));
    float3 gc = Hash33(fmod(p + float3(0.0f, 1.0f, 0.0f), freq));
    float3 gd = Hash33(fmod(p + float3(1.0f, 1.0f, 0.0f), freq));
    float3 ge = Hash33(fmod(p + float3(0.0f, 0.0f, 1.0f), freq));
    float3 gf = Hash33(fmod(p + float3(1.0f, 0.0f, 1.0f), freq));
    float3 gg = Hash33(fmod(p + float3(0.0f, 1.0f, 1.0f), freq));
    float3 gh = Hash33(fmod(p + float3(1.0f, 1.0f, 1.0f), freq));
    
    // projections
    float va = dot(ga, w - float3(0.0f, 0.0f, 0.0f));
    float vb = dot(gb, w - float3(1.0f, 0.0f, 0.0f));
    float vc = dot(gc, w - float3(0.0f, 1.0f, 0.0f));
    float vd = dot(gd, w - float3(1.0f, 1.0f, 0.0f));
    float ve = dot(ge, w - float3(0.0f, 0.0f, 1.0f));
    float vf = dot(gf, w - float3(1.0f, 0.0f, 1.0f));
    float vg = dot(gg, w - float3(0.0f, 1.0f, 1.0f));
    float vh = dot(gh, w - float3(1.0f, 1.0f, 1.0f));
	
    // interpolation
    return va +
           u.x * (vb - va) +
           u.y * (vc - va) +
           u.z * (ve - va) +
           u.x * u.y * (va - vb - vc + vd) +
           u.y * u.z * (va - vc - ve + vg) +
           u.z * u.x * (va - vb - ve + vf) +
           u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

// Tileable 3D worley noise
float WorleyNoise(float3 uv, float freq)
{
    float3 id = floor(uv);
    float3 p = frac(uv);
    
    float minDist = 10000.0f;
    for (float x = -1.0f; x <= 1.0f; ++x)
    {
        for (float y = -1.0f; y <= 1.0f; ++y)
        {
            for (float z = -1.0f; z <= 1.0f; ++z)
            {
                float3 offset = float3(x, y, z);
                float3 h = Hash33(fmod(id + offset, float3(freq, freq, freq))) * 0.5f + 0.5f;
                h += offset;
                float3 d = p - h;
                minDist = min(minDist, dot(d, d));
            }
        }
    }
    
    // inverted worley noise
    return 1.0f - minDist;
}

// Fbm(Fractional Brownian motion) for Perlin noise based on iq's blog
float Perlinfbm(float3 p, float freq, int octaves)
{
    float G = exp2(-0.85f);
    float amp = 1.0f;
    float noise = 0.0f;
    
    [unroll]
    for (int i = 0; i < octaves; ++i)
    {
        noise += amp * GradientNoise(p * freq, freq);
        freq *= 2.0f;
        amp *= G;
    }
    
    return noise;
}

// Tileable Worley fbm inspired by Andrew Schneider's Real-Time Volumetric Cloudscapes
// chapter in GPU Pro 7.
float WorleyFbm(float3 p, float freq)
{
    return WorleyNoise(p * freq, freq) * 0.625f +
        	 WorleyNoise(p * freq * 2.0f, freq * 2.0f) * 0.25f +
        	 WorleyNoise(p * freq * 4.0f, freq * 4.0f) * 0.125f;
}

#endif