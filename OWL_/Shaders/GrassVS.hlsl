#include "Common.hlsli"

struct GrassVertexInput
{
    float3 ModelPosition : POSITION;
    float3 ModelNormal : NORMAL;
    float2 Texcoord : TEXCOORD;
    matrix WorldInstance : WORLD; // Instance World
    float WindStrength : COLOR; // Const로 이동, 여기서는 테스트 용도
};

struct GrassPixelInput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 WorldNormal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 BaseColor : COLOR;
};

static float3 debugColors[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// https://thebookofshaders.com/13/
float WaveFunc1(float x, float uTime)
{
    // 여러 가지 경우에 대해 보여주기
    return sin(x + uTime);
    
    float amplitude = 1.0f;
    float frequency = 0.5f;
    
    float y = sin(x * frequency);
    float t = 0.01f * (-uTime * 130.0f);
    y += sin(x * frequency * 2.1f + t) * 4.5f;
    y += sin(x * frequency * 1.72f + t * 1.121f) * 4.0f;
    y += sin(x * frequency * 2.221f + t * 0.437f) * 5.0f;
    y += sin(x * frequency * 3.1122f + t * 4.269f) * 2.5f;
    y *= amplitude * 0.06f;
    
    return y;
}

float WaveFunc2(float x, float uTime)
{
    return 0.0f;
    
    //float amplitude = 1.0f;
    //float frequency = 0.1f;
    
    //float y = sin(x * frequency);
    //float t = 0.01f * (-uTime * 130.0f);
    //y += sin(x * frequency * 2.1f + t) * 4.5f;
    //y += sin(x * frequency * 1.72f + t * 1.121f) * 4.0f;
    //y += sin(x * frequency * 2.221f + t * 0.437f) * 5.0f;
    //y += sin(x * frequency * 3.1122f + t * 4.269f) * 2.5f;
    //y *= amplitude * 0.06;
    
    //return y;
}

// Quaternion structure for HLSL
// https://gist.github.com/mattatz/40a91588d5fb38240403f198a938a593

// A given angle of rotation about a given axis.
float4 RotateAngleAxis(float angle, float3 axis)
{
    float sn = sin(angle * 0.5f);
    float cs = cos(angle * 0.5f);
    return float4(axis * sn, cs);
}

float4x4 QuaternionToMatrix(float4 quat)
{
    float4x4 m = float4x4(float4(0.0f, 0.0f, 0.0f, 0.0f), 
                          float4(0.0f, 0.0f, 0.0f, 0.0f), 
                          float4(0.0f, 0.0f, 0.0f, 0.0f), 
                          float4(0.0f, 0.0f, 0.0f, 0.0f));

    float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    m[0][0] = 1.0f - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;

    m[1][0] = xy + wz;
    m[1][1] = 1.0f - (xx + zz);
    m[1][2] = yz - wx;

    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0f - (xx + yy);

    m[3][3] = 1.0f;

    return m;
}

GrassPixelInput main(uint instanceID : SV_InstanceID, // 참고/디버깅용
                     GrassVertexInput input)
{
    GrassPixelInput output;
    
    // 편의상 worldIT == world 라고 가정 (isotropic scaling)
    // 주의: input.insWorld, world 두 번 변환.

    output.WorldPosition = mul(float4(input.ModelPosition, 1.0f), input.WorldInstance).xyz;
    
    // Deform by wind
    float4x4 mWind = float4x4(1.0f, 0.0f, 0.0f, 0.0f, 
                              0.0f, 1.0f, 0.0f, 0.0f, 
                              0.0f, 0.0f, 1.0f, 0.0f, 
                              0.0f, 0.0f, 0.0f, 1.0f);

    float3 windWorld = float3(WaveFunc1(output.WorldPosition.x, g_GlobalTime), 0.0f, WaveFunc2(output.WorldPosition.z, g_GlobalTime + 123.0f)) * input.WindStrength;
    
    float2 rotCenter = float2(0.0f, 0.1f);
    float2 temp = (input.ModelPosition.xy - rotCenter);
    float coeff = pow(max(0, temp.y), 2.0f);
    float3 axis = cross(coeff * windWorld, float3(0.0f, 1.0f, 0.0f));
    float4 q = RotateAngleAxis(input.WindStrength, axis);
    mWind = QuaternionToMatrix(q);


    /*float2 rotCenter = float2(0.0f, -0.5f);
    float2 temp = (input.posModel.xy - rotCenter);
    float coeff = windTrunk * pow(max(0, temp.y), 2.0) * sin(globalTime);
    float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
    input.posModel.xy = mul(temp, rot);
    input.posModel.xy += rotCenter;*/
    
    output.WorldNormal = mul(float4(input.ModelNormal, 0.0f), input.WorldInstance).xyz;
    output.WorldNormal = mul(float4(output.WorldNormal, 0.0f), mWind).xyz;
    output.WorldNormal = mul(float4(output.WorldNormal, 0.0f), g_World).xyz;
    output.WorldNormal = normalize(output.WorldNormal);
    
    float3 translation = input.WorldInstance._m30_m31_m32;
    output.WorldPosition -= translation;
    output.WorldPosition = mul(float4(output.WorldPosition, 1.0f), mWind).xyz;
    output.WorldPosition += translation;
    
    output.WorldPosition = mul(float4(output.WorldPosition, 1.0f), g_World).xyz;
    output.ProjectedPosition = mul(float4(output.WorldPosition, 1.0f), g_ViewProjection);
    output.Texcoord = input.Texcoord;

    output.BaseColor = float3(0.0f, 1.0f, 0.0f) * pow(saturate(input.Texcoord.y), 3.0f);
    // output.baseColor = debugColors[instanceID % 3] * pow(saturate(input.texcoord.y), 3.0f);
    
    return output;
}
