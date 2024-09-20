#ifndef POSTCOLOR_HLSL
#define POSTCOLOR_HLSL

static const float POSTCOLOR_A = 2.51f;
static const float POSTCOLOR_B = 0.03f;
static const float POSTCOLOR_C = 2.43f;
static const float POSTCOLOR_D = 0.59f;
static const float POSTCOLOR_E = 0.14f;

float3 Tonemap(float3 input)
{
    return (input * (POSTCOLOR_A * input + POSTCOLOR_B)) / (input * (POSTCOLOR_C * input + POSTCOLOR_D) + POSTCOLOR_E);
}

float3 PostProcessColor(float2 seed, float3 input)
{
    input = Tonemap(input);

    float rand = frac(sin(dot(seed, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f);
    input = 255.0f * saturate(pow(input, 1.0f / 2.2f));
    input = (rand.xxx < (input - floor(input)) ? ceil(input) : floor(input));

    return input / 255.0f;
}

#endif
