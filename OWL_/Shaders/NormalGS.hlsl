#include "Common.hlsli"

struct NormalGeometryShaderInput
{
    float4 ModelPosition : SV_POSITION;
    float3 ModelNormal : NORMAL;
};

struct NormalPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 Color : COLOR;
};

static const float LINE_SCALE = 0.02f;

[maxvertexcount(2)]
void main(point NormalGeometryShaderInput input[1], inout LineStream<NormalPixelShaderInput> outputStream)
{
    NormalPixelShaderInput output;
    
    float4 worldPos = mul(input[0].ModelPosition, g_World);
    float4 modelNormal = float4(input[0].ModelNormal, 0.0f);
    float4 worldNormal = mul(modelNormal, g_WorldInverseTranspose);
    worldNormal = float4(normalize(worldNormal.xyz), 0.0f);
    
    output.ProjectedPosition = mul(worldPos, g_ViewProjection);
    output.Color = float3(1.0f, 1.0f, 0.0f);
    outputStream.Append(output);
    
    output.ProjectedPosition = mul(worldPos + LINE_SCALE * worldNormal, g_ViewProjection);
    output.Color = float3(1.0f, 0.0f, 0.0f);
    outputStream.Append(output);
}
