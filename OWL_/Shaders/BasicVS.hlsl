#include "Common.hlsli"

Texture2D g_HeightTex : register(t6);

PixelShaderInput main(VertexShaderInput input)
{
    // �� ��ǥ��� NDC�̱� ������ ���� ��ǥ�� �̿��ؼ� ���� ���.
    PixelShaderInput output;
    
#ifdef SKINNED
    float weights[8];
    weights[0] = input.BoneWeights0.x;
    weights[1] = input.BoneWeights0.y;
    weights[2] = input.BoneWeights0.z;
    weights[3] = input.BoneWeights0.w;
    weights[4] = input.BoneWeights1.x;
    weights[5] = input.BoneWeights1.y;
    weights[6] = input.BoneWeights1.z;
    weights[7] = input.BoneWeights1.w;
    
    uint indices[8];
    indices[0] = input.BoneIndices0.x;
    indices[1] = input.BoneIndices0.y;
    indices[2] = input.BoneIndices0.z;
    indices[3] = input.BoneIndices0.w;
    indices[4] = input.BoneIndices1.x;
    indices[5] = input.BoneIndices1.y;
    indices[6] = input.BoneIndices1.z;
    indices[7] = input.BoneIndices1.w;

    float3 modelPos = float3(0.0f, 0.0f, 0.0f);
    float3 modelNormal = float3(0.0f, 0.0f, 0.0f);
    float3 modelTangent = float3(0.0f, 0.0f, 0.0f);
    
    // Uniform Scaling ����.
    // (float3x3)boneTransforms ĳ�������� Translation ����.
    for (int i = 0; i < 8; ++i)
    {
        // weight�� ���� 1��.
        modelPos += weights[i] * mul(float4(input.ModelPosition, 1.0f), g_BoneTransforms[indices[i]]).xyz;
        modelNormal += weights[i] * mul(input.ModelNormal, (float3x3)g_BoneTransforms[indices[i]]);
        modelTangent += weights[i] * mul(input.ModelTangent, (float3x3)g_BoneTransforms[indices[i]]);
    }

    input.ModelPosition = modelPos;
    input.ModelNormal = modelNormal;
    input.ModelTangent = modelTangent;
#endif

    //����: windTrunk, windLeaves �ɼǵ� skinnedMeshó�� ��ũ�� ��� ����.
    // ���� �ٱⰡ ��鸮�� ���� ����.
    if (g_WindTrunk != 0.0f)
    {
        float2 rotationCenter = float2(0.0f, -0.5f); // �߾��� (0, 0)��. �̴� �� ��ǥ�� ����.
        float2 temp = input.ModelPosition.xy - rotationCenter;
        float coeff = g_WindTrunk * pow(max(0, temp.y), 2.0f) * sin(g_GlobalTime);
        
        float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
        
        input.ModelPosition.xy = mul(temp, rot);
        input.ModelPosition.xy += rotationCenter;
    }
    
    // �������� ��鸮�� ���� ����.
    if (g_WindLeaves != 0.0f)
    {
        float3 windVec = float3(sin(input.ModelPosition.x * 100.0 + g_GlobalTime * 0.1f) * cos(input.ModelPosition.y * 100.0f + g_GlobalTime * 0.1f), 0.0f, 0.0f) * sin(g_GlobalTime * 10.0f);
        float3 coeff = (1.0f - input.Texcoord.y) * // �ؽ��� ��ǥ ���� ���̰� 0�� �������� �� ũ�� ȸ��.
                        g_WindLeaves * dot(input.ModelNormal, windVec) * // �ٶ� ������ ���� ���� ���Ϳ� �̷�� ������ Ŭ���� �� ���ϰ� ��鸲.
                        input.ModelNormal; // ���� ���� �������� ������.
        
        input.ModelPosition.xyz += coeff;
    }
    
    output.ModelPosition = input.ModelPosition;
    output.WorldNormal = mul(float4(input.ModelNormal, 0.0f), g_WorldInverseTranspose).xyz;
    output.WorldNormal = normalize(output.WorldNormal);
    output.WorldPosition = mul(float4(input.ModelPosition, 1.0f), g_World).xyz;

    if (bUseHeightMap)
    {
        float height = g_HeightTex.SampleLevel(g_LinearClampSampler, input.Texcoord, 0.0f).r;
        height = height * 2.0f - 1.0f;
        output.WorldPosition += output.WorldNormal * height * g_HeightScale;
    }

    output.ProjectedPosition = mul(float4(output.WorldPosition, 1.0f), g_ViewProjection);
    output.Texcoord = input.Texcoord;
    output.WorldTangent = mul(float4(input.ModelTangent, 0.0f), g_World).xyz;

    return output;
}
