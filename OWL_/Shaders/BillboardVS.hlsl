#include "Common.hlsli"

struct BillboardVertexShaderInput
{
    float4 ModelPosition : POSITION; // 모델 좌표계의 위치 position
};

struct GeometryShaderInput
{
    float4 WorldPosition : SV_POSITION; // Screen position
};

GeometryShaderInput main(BillboardVertexShaderInput input)
{
    GeometryShaderInput output;
    // Geometry shader로 그대로 넘겨줌.
    output.WorldPosition = mul(input.ModelPosition, g_World);
    
    return output;
}