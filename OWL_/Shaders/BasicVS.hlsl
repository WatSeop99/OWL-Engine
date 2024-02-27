#include "Common.hlsli"

Texture2D g_HeightTex : register(t6);

PixelShaderInput main(VertexShaderInput input)
{
    // 뷰 좌표계는 NDC이기 때문에 월드 좌표를 이용해서 조명 계산.
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
    
    // Uniform Scaling 가정.
    // (float3x3)boneTransforms 캐스팅으로 Translation 제외.
    for (int i = 0; i < 8; ++i)
    {
        // weight의 합은 1임.
        modelPos += weights[i] * mul(float4(input.ModelPosition, 1.0f), g_BoneTransforms[indices[i]]).xyz;
        modelNormal += weights[i] * mul(input.ModelNormal, (float3x3)g_BoneTransforms[indices[i]]);
        modelTangent += weights[i] * mul(input.ModelTangent, (float3x3)g_BoneTransforms[indices[i]]);
    }

    input.ModelPosition = modelPos;
    input.ModelNormal = modelNormal;
    input.ModelTangent = modelTangent;
#endif

    //참고: windTrunk, windLeaves 옵션도 skinnedMesh처럼 매크로 사용 가능.
    // 나무 줄기가 흔들리는 정도 결정.
    if (g_WindTrunk != 0.0f)
    {
        float2 rotationCenter = float2(0.0f, -0.5f); // 중앙이 (0, 0)임. 이는 모델 좌표계 기준.
        float2 temp = input.ModelPosition.xy - rotationCenter;
        float coeff = g_WindTrunk * pow(max(0, temp.y), 2.0f) * sin(g_GlobalTime);
        
        float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
        
        input.ModelPosition.xy = mul(temp, rot);
        input.ModelPosition.xy += rotationCenter;
    }
    
    // 나뭇잎이 흔들리는 정도 결정.
    if (g_WindLeaves != 0.0f)
    {
        float3 windVec = float3(sin(input.ModelPosition.x * 100.0 + g_GlobalTime * 0.1f) * cos(input.ModelPosition.y * 100.0f + g_GlobalTime * 0.1f), 0.0f, 0.0f) * sin(g_GlobalTime * 10.0f);
        float3 coeff = (1.0f - input.Texcoord.y) * // 텍스쳐 좌표 기준 높이가 0에 가까울수록 더 크게 회전.
                        g_WindLeaves * dot(input.ModelNormal, windVec) * // 바람 방향이 잎의 수직 벡터와 이루는 각도가 클수록 더 강하게 흔들림.
                        input.ModelNormal; // 수직 벡터 방향으로 움직임.
        
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
