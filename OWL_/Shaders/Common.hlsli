#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SUN 0x08
#define LIGHT_SHADOW 0x10

// 샘플러들을 모든 쉐이더에서 공통으로 사용
SamplerState g_LinearWrapSampler : register(s0);
SamplerState g_LinearClampSampler : register(s1);
SamplerState g_ShadowPointSampler : register(s2);
SamplerState g_ShadowLinearSampler : register(s3);
SamplerComparisonState g_ShadowCompareSampler : register(s4);
SamplerState g_PointWrapSampler : register(s5);
SamplerState g_LinearMirrorSampler : register(s6);
SamplerState g_PointClampSampler : register(s7);

// 공용 텍스춰들 t10 부터 시작
TextureCube g_EnvIBLTex : register(t10);
TextureCube g_SpecularIBLTex : register(t11);
TextureCube g_IrradianceIBLTex : register(t12);
Texture2D g_BRDFTex : register(t13);

struct Light
{
    float3 Radiance; // Strength
    float FallOffStart;
    float3 Direction;
    float FallOffEnd;
    float3 Position;
    float SpotPower;
    
    uint Type;
    float Radius;
    float HaloRadius;
    float HaloStrength;

    matrix ViewProjection[6];
    matrix Projections[4];
    matrix InverseProjections[4];
};

// 공용 Constants
cbuffer GlobalConstants : register(b0)
{
    matrix g_View;
    matrix g_Projection;
    matrix g_InverseProjection;
    matrix g_ViewProjection;
    matrix g_InverseViewProjection; // Proj -> World
    matrix g_InverseView;

    float3 g_EyeWorld;
    float g_StrengthIBL;

    int g_TextureToDraw; // 0: Env, 1: Specular, 2: Irradiance, 그외: 검은색
    float g_EnvLodBias; // 환경맵 LodBias
    float g_LODBias; // 다른 물체들 LodBias
    float g_GlobalTime;
    
    int dummy[4];
};
cbuffer LightConstants : register(b1)
{
    Light lights;
};
cbuffer MeshConstants : register(b2)
{
    matrix g_World; // Model(또는 Object) 좌표계 -> World로 변환
    matrix g_WorldInverseTranspose; // World의 InverseTranspose
    matrix g_WorldInverse;
    bool bUseHeightMap;
    float g_HeightScale;
    float g_WindTrunk;
    float g_WindLeaves;
};
cbuffer MaterialConstants : register(b3)
{
    float3 g_AlbedoFactor; // baseColor
    float g_RoughnessFactor;
    float g_MetallicFactor;
    float3 g_EmissionFactor;

    bool bUseAlbedoMap;
    bool bUseNormalMap;
    bool bUseAOMap; // Ambient Occlusion
    bool bInvertNormalMapY;
    bool bUseMetallicMap;
    bool bUseRoughnessMap;
    bool bUseEmissiveMap;
    float dummy2;
};

#ifdef SKINNED
// 관절 개수 제약을 없애게 위해 StructuredBuffer 사용
StructuredBuffer<matrix> g_BoneTransforms : register(t9);
#endif

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
};

#endif