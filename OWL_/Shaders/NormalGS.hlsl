#include "Common.hlsli"

struct NormalGeometryShaderInput
{
    float4 posModel : SV_POSITION;
    float3 normalModel : NORMAL;
};

struct NormalPixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

static const float LINE_SCALE = 0.02f;

[maxvertexcount(2)]
void main(point NormalGeometryShaderInput input[1], inout LineStream<NormalPixelShaderInput> outputStream)
{
    NormalPixelShaderInput output;
    
    float4 posWorld = mul(input[0].posModel, world);
    float4 normalModel = float4(input[0].normalModel, 0.0f);
    float4 normalWorld = mul(normalModel, worldIT);
    normalWorld = float4(normalize(normalWorld.xyz), 0.0f);
    
    output.pos = mul(posWorld, viewProj);
    output.color = float3(1.0f, 1.0f, 0.0f);
    outputStream.Append(output);
    
    output.pos = mul(posWorld + LINE_SCALE * normalWorld, viewProj);
    output.color = float3(1.0f, 0.0f, 0.0f);
    outputStream.Append(output);
}
