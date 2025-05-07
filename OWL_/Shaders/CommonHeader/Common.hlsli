#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

// 모든 리소스 버퍼 순서는 다음과 같음.
// constant resource (b0~)
// texture resource (t0~)
// sampler resource (s0~)
// unordered read-write resource (u0~)

#include "CommonHeader/GlobalResource.hlsli"
#include "CommonHeader/LightResource.hlsli"
#include "CommonHeader/ObjectResource.hlsli"

struct VertexShaderInput
{
    float3 ModelPosition : POSITION; //모델 좌표계의 위치 position
    float3 ModelNormal : NORMAL; // 모델 좌표계의 normal    
    float2 Texcoord : TEXCOORD;
    float3 ModelTangent : TANGENT;
#ifdef SKINNED
    float4 BoneWeights0 : BLENDWEIGHT0;
    float4 BoneWeights1 : BLENDWEIGHT1;
    uint4 BoneIndices0 : BLENDINDICES0;
    uint4 BoneIndices1 : BLENDINDICES1;
#endif
};
struct PixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION; // Screen position
    float3 WorldPosition : POSITION0; // World position (조명 계산에 사용)
    float3 WorldNormal : NORMAL0;
    float2 Texcoord : TEXCOORD0;
    float3 WorldTangent : TANGENT0;
    float3 ModelPosition : POSITION1; // Volume casting 시작점
    float4 Color : COLOR;
};
typedef PixelShaderInput VertexShaderOutput;

#endif
