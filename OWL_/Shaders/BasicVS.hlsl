#include "Common.hlsli"

// Vertex Shader에서도 텍스춰 사용.
Texture2D g_heightTexture : register(t0);

PixelShaderInput main(VertexShaderInput input)
{
    // 뷰 좌표계는 NDC이기 때문에 월드 좌표를 이용해서 조명 계산.
    PixelShaderInput output;
    
#ifdef SKINNED
    // 참고 자료: Luna DX 12 교재.
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
    
    // Uniform Scaling 가정.
    // (float3x3)boneTransforms 캐스팅으로 Translation 제외.
    for (int i = 0; i < 8; ++i)
    {
        // weight의 합은 1임.
        posModel += weights[i] * mul(float4(input.posModel, 1.0f), boneTransforms[indices[i]]).xyz;
        normalModel += weights[i] * mul(input.normalModel, (float3x3)boneTransforms[indices[i]]);
        tangentModel += weights[i] * mul(input.tangentModel, (float3x3)boneTransforms[indices[i]]);
    }

    input.posModel = posModel;
    input.normalModel = normalModel;
    input.tangentModel = tangentModel;

#endif

    //참고: windTrunk, windLeaves 옵션도 skinnedMesh처럼 매크로 사용 가능.
    // 나무 줄기가 흔들리는 정도 결정.
    if (windTrunk != 0.0f)
    {
        float2 rotationCenter = float2(0.0f, -0.5f); // 중앙이 (0, 0)임. 이는 모델 좌표계 기준.
        float2 temp = input.posModel.xy - rotationCenter;
        float coeff = windTrunk * pow(max(0, temp.y), 2.0f) * sin(globalTime);
        
        float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
        
        input.posModel.xy = mul(temp, rot);
        input.posModel.xy += rotationCenter;
    }
    
    // 나뭇잎이 흔들리는 정도 결정.
    if (windLeaves != 0.0f)
    {
        float3 windVec = float3(sin(input.posModel.x * 100.0 + globalTime * 0.1f) * cos(input.posModel.y * 100.0f + globalTime * 0.1f), 
                                0.0f, 
                                0.0f) * sin(globalTime * 10.0f);
        float3 coeff = (1.0f - input.texcoord.y) * // 텍스쳐 좌표 기준 높이가 0에 가까울수록 더 크게 회전.
                        windLeaves * dot(input.normalModel, windVec) * // 바람 방향이 잎의 수직 벡터와 이루는 각도가 클수록 더 강하게 흔들림.
                        input.normalModel; // 수직 벡터 방향으로 움직임.
        
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
