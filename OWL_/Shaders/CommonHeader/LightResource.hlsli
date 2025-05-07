#ifndef __LIGHT_RESOURCE__
#define __LIGHT_RESOURCE__

#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SUN 0x08
#define LIGHT_SHADOW 0x10

struct Light
{
    float3 Radiance; // Strength
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
    float3 Position;
    float SpotPower;
    
    uint Type;
    float Radius;
    float HaloRadius;
    float HaloStrength;

    matrix ViewProjection[6];
    matrix Projections[4];
    matrix InverseProjections[4];
};

cbuffer LightConstants : register(b1)
{
    Light lights;
};

#endif
