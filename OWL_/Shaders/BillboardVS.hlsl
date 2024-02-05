#include "Common.hlsli"

struct BillboardVertexShaderInput
{
    float4 pos : POSITION; // �� ��ǥ���� ��ġ position
};

struct GeometryShaderInput
{
    float4 pos : SV_POSITION; // Screen position
};

GeometryShaderInput main(BillboardVertexShaderInput input)
{
    GeometryShaderInput output;
    // Geometry shader�� �״�� �Ѱ���.
    output.pos = mul(input.pos, world);
    
    return output;
}