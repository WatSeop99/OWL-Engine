#ifndef __OBJECT_RESOURCE__
#define __OBJECT_RESOURCE__

cbuffer MeshConstants : register(b2)
{
    matrix g_World; // Model(또는 Object) 좌표계 -> World로 변환
    matrix g_WorldInverseTranspose; // World의 InverseTranspose
    matrix g_WorldInverse;
    bool bUseHeightMap;
    bool bUseColorMap;
    float g_HeightScale;
    float g_WindTrunk;
    float g_WindLeaves;
    float pad[3];
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

#endif