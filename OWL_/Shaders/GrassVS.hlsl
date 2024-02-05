#include "Common.hlsli"

struct GrassVertexInput
{
    float3 posModel : POSITION;
    float3 normalModel : NORMAL;
    float2 texcoord : TEXCOORD;
    matrix insWorld : WORLD; // Instance World
    float windStrength : COLOR; // Const�� �̵�, ���⼭�� �׽�Ʈ �뵵
};

struct GrassPixelInput
{
    float4 posProj : SV_POSITION;
    float3 posWorld : POSITION;
    float3 normalWorld : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 baseColor : COLOR;
};

static float3 debugColors[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

// https://thebookofshaders.com/13/
float WaveFunc1(float x, float u_time)
{
    // ���� ���� ��쿡 ���� �����ֱ�
    return sin(x + u_time);
    
    float amplitude = 1.0f;
    float frequency = 0.5f;
    
    float y = sin(x * frequency);
    float t = 0.01f * (-u_time * 130.0f);
    y += sin(x * frequency * 2.1f + t) * 4.5f;
    y += sin(x * frequency * 1.72f + t * 1.121f) * 4.0f;
    y += sin(x * frequency * 2.221f + t * 0.437f) * 5.0f;
    y += sin(x * frequency * 3.1122f + t * 4.269f) * 2.5f;
    y *= amplitude * 0.06f;
    
    return y;
}

float WaveFunc2(float x, float u_time)
{
    return 0.0f;
    
    //float amplitude = 1.0f;
    //float frequency = 0.1f;
    
    //float y = sin(x * frequency);
    //float t = 0.01f * (-u_time * 130.0f);
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

GrassPixelInput main(uint instanceID : SV_InstanceID, // ����/������
                     GrassVertexInput input)
{
    GrassPixelInput output;
    
    // ���ǻ� worldIT == world ��� ���� (isotropic scaling)
    // ����: input.insWorld, world �� �� ��ȯ.

    output.posWorld = mul(float4(input.posModel, 1.0f), input.insWorld).xyz;
    
    // Deform by wind
    float4x4 mWind = float4x4(1.0f, 0.0f, 0.0f, 0.0f, 
                              0.0f, 1.0f, 0.0f, 0.0f, 
                              0.0f, 0.0f, 1.0f, 0.0f, 
                              0.0f, 0.0f, 0.0f, 1.0f);

    float3 windWorld = float3(WaveFunc1(output.posWorld.x, globalTime), 0.0f, WaveFunc2(output.posWorld.z, globalTime + 123.0f)) * input.windStrength;
    
    float2 rotCenter = float2(0.0f, 0.1f);
    float2 temp = (input.posModel.xy - rotCenter);
    float coeff = pow(max(0, temp.y), 2.0f);
    float3 axis = cross(coeff * windWorld, float3(0.0f, 1.0f, 0.0f));
    float4 q = RotateAngleAxis(input.windStrength, axis);
    mWind = QuaternionToMatrix(q);


    /*float2 rotCenter = float2(0.0f, -0.5f);
    float2 temp = (input.posModel.xy - rotCenter);
    float coeff = windTrunk * pow(max(0, temp.y), 2.0) * sin(globalTime);
    float2x2 rot = float2x2(cos(coeff), sin(coeff), -sin(coeff), cos(coeff));
    input.posModel.xy = mul(temp, rot);
    input.posModel.xy += rotCenter;*/
    
    output.normalWorld = mul(float4(input.normalModel, 0.0f), input.insWorld).xyz;
    output.normalWorld = mul(float4(output.normalWorld, 0.0f), mWind).xyz;
    output.normalWorld = mul(float4(output.normalWorld, 0.0f), world).xyz;
    output.normalWorld = normalize(output.normalWorld);
    
    float3 translation = input.insWorld._m30_m31_m32;
    output.posWorld -= translation;
    output.posWorld = mul(float4(output.posWorld, 1.0f), mWind).xyz;
    output.posWorld += translation;
    
    output.posWorld = mul(float4(output.posWorld, 1.0f), world).xyz;
    output.posProj = mul(float4(output.posWorld, 1.0f), viewProj);
    output.texcoord = input.texcoord;

    output.baseColor = float3(0.0f, 1.0f, 0.0f) * pow(saturate(input.texcoord.y), 3.0f);
    // output.baseColor = debugColors[instanceID % 3] * pow(saturate(input.texcoord.y), 3.0f);
    
    return output;
}