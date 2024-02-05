#include "Common.hlsli"

struct DepthOnlyPixelShaderInput
{
    float4 posProj : SV_POSITION;
};

void main(float4 pos : SV_POSITION)
{
    // 아무것도 하지 않음 (Depth Only)
}
