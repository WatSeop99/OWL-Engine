#ifndef __GLOBAL_RESOURCE__
#define __GLOBAL_RESOURCE__

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

TextureCube g_EnvIBLTex : register(t10);
TextureCube g_SpecularIBLTex : register(t11);
TextureCube g_IrradianceIBLTex : register(t12);
Texture2D g_BRDFTex : register(t13);

SamplerState g_LinearWrapSampler : register(s0);
SamplerState g_LinearClampSampler : register(s1);
SamplerState g_ShadowPointSampler : register(s2);
SamplerState g_ShadowLinearSampler : register(s3);
SamplerComparisonState g_ShadowCompareSampler : register(s4);
SamplerState g_PointWrapSampler : register(s5);
SamplerState g_LinearMirrorSampler : register(s6);
SamplerState g_PointClampSampler : register(s7);

#endif
