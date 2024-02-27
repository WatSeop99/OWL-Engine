#include "Common.hlsli"

struct BillboardVertexShaderInput
{
    float4 ModelPosition : POSITION; // �� ��ǥ���� ��ġ position
};

struct GeometryShaderInput
{
    float4 WorldPosition : SV_POSITION; // Screen position
};

GeometryShaderInput main(BillboardVertexShaderInput input)
{
    GeometryShaderInput output;
    // Geometry shader�� �״�� �Ѱ���.
    output.WorldPosition = mul(input.ModelPosition, g_World);
    
    return output;
}