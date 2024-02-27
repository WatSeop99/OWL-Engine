#include "common.hlsli"
#include "Quaternion.hlsli"

cbuffer BillboardContsts : register(b3)
{
    float g_WidthWorld;
    float3 g_DirWorld;
};

struct BillboardPixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float4 WorldPosition : POSITION0;
    float4 Center : POSITION1;
    float2 Texcoord : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};
struct PixelShaderOutput
{
    float4 PixelColor : SV_Target0;
};

// BRADY'S VOLUMETRIC FIRE
// https://www.shadertoy.com/view/WllXzB

float Noise(float3 P)
{
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0f;
    Pi.xyz = Pi.xyz - floor(Pi.xyz * (1.0f / 69.0f)) * 69.0f;
    float3 temp;
    temp.xyz = 69.0f - 1.5f;
    float3 Pi_inc1 = step(Pi, temp) * (Pi + 1.0f);
    float4 Pt = float4(Pi.xy, Pi_inc1.xy) + float2(50.0f, 161.0f).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float2 hash_mod = float2(1.0f / (635.298681f + float2(Pi.z, Pi_inc1.z) * 48.500388f));
    float4 hash_lowz = frac(Pt * hash_mod.xxxx);
    float4 hash_highz = frac(Pt * hash_mod.yyyy);
    float3 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0f - 15.0f) + 10.0f);
    float4 res0 = lerp(hash_lowz, hash_highz, blend.z);
    float4 blend2 = float4(blend.xy, float2(1.0 - blend.xy));
    return dot(res0, blend2.zxzx * blend2.wwyy);
}

float FNoise(float3 p, float time)
{
    float f = 0.0f;
    //p = p - float3(0.0, 0.0, 1.0) * time;
    p = p + g_DirWorld * time; // 진행방향 (반대로) 꼬리 흐름 만들기
    p = p * 3.0f;
    f += 0.50000 * Noise(p);
    p = 2.0f * p;
    f += 0.25000 * Noise(p);
    p = 2.0f * p;
    f += 0.12500f * Noise(p);
    p = 2.0f * p;
    f += 0.06250f * Noise(p);
    p = 2.0f * p;
    f += 0.03125f * Noise(p);
    
    return f;
}

float3x3 AngleAxis3x3(float angle, float3 axis)
{
    float c, s;
    sincos(angle, s, c);

    float t = 1.0f - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return float3x3(t * x * x + c, t * x * y - s * z, t * x * z + s * y,
                    t * x * y + s * z, t * y * y + c, t * y * z - s * x,
                    t * x * z - s * y, t * y * z + s * x, t * z * z + c);
}

float ModelFunc(in float3 p)
{
    //p.x *= .7;

    //p.z *= 0.3;
    //float4 rotQuat = from_to_rotation(normalize(-dirWorld), float3(0, 0, 1) );
    //p = mul(float4(p, 0.0), quaternion_to_matrix(rotQuat));

    float sphere = length(p) - 0.8f; // 중심이 원점인 구
    float res = sphere + FNoise(p * 1.5f, g_GlobalTime * 5.0f) * 0.4f;
    //float res = sphere;
    
    //float res = sphere + fnoise(p * 1.5, 0 * 3.) * .4;
    return res * 0.8f;
}

float Raymarch(in float3 ro, in float3 rd)
{
    float dist = 0.0f;
    for (int i = 0; i < 30; ++i)
    {
        float m = ModelFunc(ro + rd * dist);
        dist += m;
        
        if (m < 0.01f) // 구 안으로 들어갔다면 거리 반환
        {
            return dist;
        }
        else if (dist > 10.0f)
        {
            break;
        }
    }
    
    return -1.0f;
}

float3 HueShift(float3 color, float hueAdjust)
{
    const float3 RGB_TO_Y_PRIME = float3(0.299f, 0.587f, 0.114f);
    const float3 RGB_TO_I = float3(0.596f, -0.275f, -0.321f);
    const float3 RGB_TP_Q = float3(0.212f, -0.523f, 0.311f);

    const float3 YIQ_TO_R = float3(1.0f, 0.956f, 0.621f);
    const float3 YIQ_TO_G = float3(1.0f, -0.272f, -0.647f);
    const float3 YIQ_TO_B = float3(1.0f, -1.107f, 1.704f);

    float YPrime = dot(color, RGB_TO_Y_PRIME);
    float I = dot(color, RGB_TO_I);
    float Q = dot(color, RGB_TP_Q);
    float hue = atan2(Q, I);
    float chroma = sqrt(I * I + Q * Q);

    hue += hueAdjust;

    Q = chroma * sin(hue);
    I = chroma * cos(hue);

    float3 yIQ = float3(YPrime, I, Q);

    return float3(dot(yIQ, YIQ_TO_R), dot(yIQ, YIQ_TO_G), dot(yIQ, YIQ_TO_B));
}

float3 Saturation(float3 rgb, float adjustment)
{
    const float3 W = float3(0.2125, 0.7154, 0.0721);
    float3 intensity;
    intensity.xyz = dot(rgb, W);
    return lerp(intensity, rgb, adjustment);
}

float3 Volume(in float3 p, in float3 rd, in float3 ld)
{
    float3 op = p;
    float trans = 1.0f;
    float td = 0.0f;
    float dif = 0.0f;
    float emit = 0.0f;
    float steps = 30.0f; // increase to smooth
    
    // march
    for (float i = 0.0f; i < steps; ++i)
    {
        float m = ModelFunc(p);
        p += rd * 0.03f;
        
        float dens = 1.0f - smoothstep(0.0f, 0.35f, -m);
        td += dens;
        trans *= dens;
        
        if (td > 1.0f && dif <= 0.0f)
        {
            td = 1.0f;
            dif = clamp(1.0f - ModelFunc(p - 0.1f * ld), 0.0f, 1.0f);
            emit = pow(smoothstep(-0.3f, 0.0f, ModelFunc(p)), 4.0f);
        }
    }
    
    trans = (1.0f - pow(abs(td / steps), 4.5f));
    trans = smoothstep(0.0f, 1.0f, trans);
    float emitmod = (emit * trans) * 0.8f + 0.2f;
    
    // light
    float3 lin = float3(0.3f, 0.2f, 0.9f);
    lin = HueShift(lin, 4.2f + -trans * 0.6f + dif * 0.5f);
    lin *= emitmod;
    
    // bright/sat/contrast
    lin = Saturation(lin, pow(trans, 0.5f) * 0.4f);
    lin *= 5.0f;
    lin -= float3(0.4f, 0.4f, 0.4f);
    
    return lerp(float3(0.0f, 0.0f, 0.0f), lin, pow(trans, 1.25f));
}

// 쉐이더 토이에서 직접 바꿔가는 과정도 설명
/*mat3 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = (fragCoord - .5 * iResolution.xy) / iResolution.y;
    p.xy *= 4.0;
    
    mat3 rot = rotationMatrix(vec3(0.0, 1.0, 0.0), iMouse.x / 100.0);
    
    //vec3 ro = normalize(vec3(cos(iMouse.x/100.), 0.0, -sin(iMouse.x/100.))) * 2.35;
    vec3 ro = vec3(0.0, 0.0, 1.35) * rot;
    vec3 rd = normalize(vec3(p.xy, 0.0) * rot - ro); // xy 평면도 같이 회전해야 함
    
    float dist = raymarch(ro, rd);
    vec3 ld = vec3(-1., 1., 0.);
    vec3 col = dist > 0. ? volume(ro + rd * dist, rd, ld, p) : background(p);
    
    fragColor = vec4(col, 1.0);
}*/

PixelShaderOutput main(BillboardPixelShaderInput input)
{
    float2 p = input.Texcoord - 0.5f;
    p.xy = p.xy * 2.0f;

    //float3 ro = normalize(mul(float4(0, 0, 1, 0), invView).xyz) * 1.35;
    //float3 rd = normalize(mul(float4(p.x, p.y, 0, 0), invView).xyz - ro);
    float3 posLocal = (input.WorldPosition.xyz - input.Center.xyz) * 10.0; // 쉐이더토이 원본 효과와 스케일 맞춰주기
    float3 eyeLocal = (g_EyeWorld - input.Center.xyz) * 10.0;
    float3 rd = normalize(posLocal - eyeLocal);
    float3 ro = posLocal - rd * 1.35;
    
    float4 rotQuat1 = FromToRotation(float3(0.0f, 0.0f, 1.0f), g_DirWorld);
    float4 rotQuat2 = FromToRotation(g_DirWorld, float3(0.0f, 0.0f, 1.0f));
    rd = mul(float4(rd, 0.0f), QuaternionToMatrix(rotQuat1)).xyz;
    ro = mul(float4(ro, 0.0f), QuaternionToMatrix(rotQuat1)).xyz;
    rd.z *= 0.5f;
    ro.z *= 0.5f;
    rd = mul(float4(rd, 0.0f), QuaternionToMatrix(rotQuat2)).xyz;
    ro = mul(float4(ro, 0.0f), QuaternionToMatrix(rotQuat2)).xyz;

    /*
    *     //p.z *= 0.3;
    //float4 rotQuat = from_to_rotation(normalize(-dirWorld), float3(0, 0, 1) );
    //p = mul(float4(p, 0.0), quaternion_to_matrix(rotQuat));
    */

    float dist = Raymarch(ro, rd);
    float3 ld = float3(-1.0f, 1.0f, 0.0f);
    float3 col = (dist > 0.0f ? Volume(ro + rd * dist, rd, ld) : float3(0.0f, 0.0f, 0.0f));
    
    PixelShaderOutput output;
    output.PixelColor = float4(col, dot(float3(1.0f, 1.0f, 1.0f), col) * 1.3f);
    //output.pixelColor.a = 1.0; // 테스트용

    return output;
}