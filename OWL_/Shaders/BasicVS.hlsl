#include "Common.hlsli"

// Vertex Shader������ �ؽ��� ���.
Texture2D g_heightTexture : register(t0);

PixelShaderInput main(VertexShaderInput input)
{
    // �� ��ǥ��� NDC�̱� ������ ���� ��ǥ�� �̿��ؼ� ���� ���.
    PixelShaderInput output;
    
#ifdef SKINNED
    // ���� �ڷ�: Luna DX 12 ����.
    float weights[8];
    weights[0] = input.boneWeights0.x;
    weights[1] = input.boneWeights0.y;
    weights[2] = input.boneWeights0.z;
    weights[3] = input.boneWeights0.w;
    weights[4] = input.boneWeights1.x;
    weights[5] = input.boneWeights1.y;
    weights[6] = input.boneWeights1.z;
    weights[7] = input.boneWeights1.w;
    
    uint indices[8];
    indices[0] = input.boneIndices0.x;
    indices[1] = input.boneIndices0.y;
    indices[2] = input.boneIndices0.z;
    indices[3] = input.boneIndices0.w;
    indices[4] = input.boneIndices1.x;
    indices[5] = input.boneIndices1.y;
    indices[6] = input.boneIndices1.z;
    indices[7] = input.boneIndices1.w;

    float3 posModel = float3(0.0f, 0.0f, 0.0f);
    float3 normalModel = float3(0.0f, 0.0f, 0.0f);
    float3 tangentModel = float3(0.0f, 0.0f, 0.0f);
    
    // Uniform Scaling ����.
    // (float3x3)boneTransforms ĳ�������� Translation ����.
    for (int i = 0; i < 8; ++i)
    {
        // weight�� ���� 1��.
        posModel += weights[i] * mul(float4(input.posModel, 1.0f), boneTransforms[indices[i]]).xyz;
        normalModel += weights[i] * mul(input.normalModel, (float3x3)boneTransforms[indices[i]]);
        tangentModel += weights[i] * mul(input.tangentModel, (float3x3)boneTransforms[indices[i]]);
    }

    input.posModel = posModel;
    input.normalModel = normalModel;
    input.tangentModel = tangentModel;

#endif

    //����: windTrunk, windLeaves �ɼǵ� skinnedMeshó�� ��ũ�� ��� ����.
    // ���� �ٱⰡ ��鸮�� ���� ����.
    if (windTrunk != 0.0f)
    {
        float2 rotationCenter = float2(0.0f, -0.5f); // �߾��� (0, 0)��. �̴� �� ��ǥ�� ����.
        float2 temp = input.posModel.xy - rotationCenter;
        float coeff = windTrunk * pow(max(0, temp.y), 2.0f) * sin(globalTime);
        
        float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
        
        input.posModel.xy = mul(temp, rot);
        input.posModel.xy += rotationCenter;
    }
    
    // �������� ��鸮�� ���� ����.
    if (windLeaves != 0.0f)
    {
        float3 windVec = float3(sin(input.posModel.x * 100.0 + globalTime * 0.1f) * cos(input.posModel.y * 100.0f + globalTime * 0.1f), 
                                0.0f, 
                                0.0f) * sin(globalTime * 10.0f);
        float3 coeff = (1.0f - input.texcoord.y) * // �ؽ��� ��ǥ ���� ���̰� 0�� �������� �� ũ�� ȸ��.
                        windLeaves * dot(input.normalModel, windVec) * // �ٶ� ������ ���� ���� ���Ϳ� �̷�� ������ Ŭ���� �� ���ϰ� ��鸲.
                        input.normalModel; // ���� ���� �������� ������.
        
        input.posModel.xyz += coeff;
    }
    
    output.posModel = input.posModel;
    output.normalWorld = mul(float4(input.normalModel, 0.0f), worldIT).xyz;
    output.normalWorld = normalize(output.normalWorld);
    output.posWorld = mul(float4(input.posModel, 1.0f), world).xyz;

    if (useHeightMap)
    {
        float height = g_heightTexture.SampleLevel(linearClampSampler, input.texcoord, 0.0f).r;
        height = height * 2.0f - 1.0f;
        output.posWorld += output.normalWorld * height * heightScale;
    }

    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;
    output.tangentWorld = mul(float4(input.tangentModel, 0.0f), world).xyz;

    return output;
}
