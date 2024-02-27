#include "Common.hlsli"

struct NormalGeometryShaderInput
{
    float4 ModelPosition : SV_POSITION;
    float3 ModelNormal : NORMAL;
};

NormalGeometryShaderInput main(VertexShaderInput input)
{
    NormalGeometryShaderInput output;

    output.ModelPosition = float4(input.ModelPosition, 1.0f);
    output.ModelNormal = input.ModelNormal;

    return output;
}
