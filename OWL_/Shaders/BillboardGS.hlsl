#include "Common.hlsli"

struct GeometryShaderInput
{
    float4 WorldPosition : SV_POSITION;
};

struct BillboardPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION; // Screen position
    float4 WorldPosition : POSITION0;
    float4 Center : POSITION1;
    float2 Texcoord : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};


// Geometry-Shader Object
// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-geometry-shader

// Stream-Output Object
// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-so-type

cbuffer BillboardContsts : register(b4)
{
    float g_WidthWorld;
    float3 g_DirWorld;
};

[maxvertexcount(4)]
void main(point GeometryShaderInput input[1], uint primID : SV_PrimitiveID, inout TriangleStream<BillboardPixelShaderInput> outputStream)
{
    float hw = 0.5f * g_WidthWorld;
    
    //float4 up = float4(0.0, 1.0, 0.0, 0.0);
    float4 up = mul(float4(0.0f, 1.0f, 0.0f, 0.0f), g_InverseView); // <- 뷰의 업벡터를 월드로 변환. (파이어볼을 위에서 보는 경우)
    up.xyz = normalize(up.xyz);
    float4 front = float4(g_EyeWorld, 1.0f) - input[0].WorldPosition;
    front.w = 0.0f; // 벡터
    
    // 빌보드가 시점을 바라보는 방향 기준으로 오른쪽.
    // 시점에서 빌보드를 바라보는 방향에서는 왼쪽. (텍스춰 좌표 주의)
    float4 right = float4(cross(up.xyz, normalize(front.xyz)), 0.0f);
    
    BillboardPixelShaderInput output;
    
    output.Center = input[0].WorldPosition; // 빌보드의 중심.
    
    output.WorldPosition = input[0].WorldPosition - hw * right - hw * up;
    output.ProjectedPosition = output.WorldPosition;
    output.ProjectedPosition = mul(output.ProjectedPosition, g_View);
    output.ProjectedPosition = mul(output.ProjectedPosition, g_Projection);
    output.Texcoord = float2(1.0f, 1.0f);
    output.PrimID = primID;
    
    outputStream.Append(output);

    output.WorldPosition = input[0].WorldPosition - hw * right + hw * up;
    output.ProjectedPosition = output.WorldPosition;
    output.ProjectedPosition = mul(output.ProjectedPosition, g_View);
    output.ProjectedPosition = mul(output.ProjectedPosition, g_Projection);
    output.Texcoord = float2(1.0f, 0.0f);
    output.PrimID = primID; // 동일.
    
    outputStream.Append(output);
    
    output.WorldPosition = input[0].WorldPosition + hw * right - hw * up;
    output.ProjectedPosition = output.WorldPosition;
    output.ProjectedPosition = mul(output.ProjectedPosition, g_View);
    output.ProjectedPosition = mul(output.ProjectedPosition, g_Projection);
    output.Texcoord = float2(0.0f, 1.0f);
    output.PrimID = primID; // 동일.
    
    outputStream.Append(output);
    
    output.WorldPosition = input[0].WorldPosition + hw * right + hw * up;
    output.ProjectedPosition = output.WorldPosition;
    output.ProjectedPosition = mul(output.ProjectedPosition, g_View);
    output.ProjectedPosition = mul(output.ProjectedPosition, g_Projection);
    output.Texcoord = float2(0.0f, 0.0f);
    output.PrimID = primID; // 동일.
    
    outputStream.Append(output);

    // 주의: GS는 Triangle Strips으로 출력합니다.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips

    outputStream.RestartStrip(); // Strip을 다시 시작.
}
