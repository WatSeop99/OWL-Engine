#include "Common.hlsli"

#define FAR_PLANE 50.0f;

struct DepthOnlyPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
};

//float4 main(DepthOnlyPixelShaderInput input) : SV_TARGET
//{
//    //float lightDistance = length(input.ProjectedPosition.xyz - lights[0].Position); // 첫번째라고 가정.
//    //lightDistance /= FAR_PLANE;
    
//    //return float4(lightDistance, 0.0f, 0.0f, 0.0f);
    
//    return float4(input.ProjectedPosition.z, 0.0f, 0.0f, 1.0f);
//}

void main(DepthOnlyPixelShaderInput input)
{ }