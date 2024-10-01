struct PixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION; // Screen position
    float4 WorldPosition : POSITION0;
    float4 Center : POSITION1;
    float2 Texcoord : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};
struct PixelShaderOutput
{
    float4 PixelColor : SV_TARGET0;
};

cbuffer BillboardModelConstantData : register(b1)
{
    float3 g_EyeWorld;
    float g_Width;
    Matrix g_Model; // For vertex shader
    Matrix g_View; // For vertex shader
    Matrix g_Projection; // For vertex shader
    float g_ObjectTime;
    float3 padding;
};

Texture2DArray g_TexArray : register(t5);

SamplerState g_Sampler : register(s0);

// Game Explosion
// https://www.shadertoy.com/view/XtVyDz

static const int LAYERS = 128;
static const float BLUR = 0.1f;
static const float SPEED = 4.0f;
static const float PEAKS = 8.0f;
static const float PEAK_STRENGTH = 0.1f;
static const float RING_SPEED = 1.5f;
static const float SMOKE = 0.4f;
static const float SMOKE_TIME = 40.0f;

float Hash(float seed)
{
    return frac(sin(seed * 3602.64325f) * 51.63891f);
}

float Circle(float radius, float2 pos)
{
    return radius - pos.x;
}

PixelShaderOutput main(PixelShaderInput input)
{
    float time = g_ObjectTime * 1.5f;
    float2 uv = (input.Texcoord * 2.0f - 1.0f) * 4.0f;
    float2 puv = float2(length(uv), atan2(uv.x, uv.y)); //polar coordinates.

    float3 col = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < LAYERS; ++i)
    {
        float prog = (float) i / (float) LAYERS;
        float radius = prog * ((1.0f - 1.0f / pow(abs(time * SPEED), 1.0f / 3.0f)) * 2.0f); //decrease radius using cubed
        radius += sin(puv.y * PEAKS + Hash(prog) * 513.0f) * PEAK_STRENGTH; //modulate radius so it isnt enitly symetrical
        float3 color = float3(1.0f / radius, 0.25f, 0.1f * (2.0f - radius)) / time / abs(log(time * RING_SPEED) - puv.x); // base explosion color, decrease with time and with distance from center
        float3 temp;
        temp.rgb = (1.0f - time / SMOKE_TIME) * puv.x * SMOKE;
        color += temp; //add smoke color, falloff can be controlled with smoketime variable
        col += color * smoothstep(0.0f, 1.0f, Circle(radius, puv) / BLUR);
    }

    col /= (float) LAYERS;
    
    PixelShaderOutput output;
    float alpha = saturate(smoothstep(dot(float3(1.0f, 1.0f, 1.0f), col), 0.0f, 1.0f));
    output.PixelColor = float4(col.rgb, alpha);
    return output;
}

