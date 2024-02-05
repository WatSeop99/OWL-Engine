#include "Common.hlsli"

#define PI 3.141592f

Texture3D<float> densityTex : register(t5); // t5 ���� ����
Texture3D<float> lightingTex : register(t6);
Texture3D<float> temperatureTex : register(t7);

cbuffer Consts : register(b3) // b3 ����
{
    float3 uvwOffset; // �̻��
    float lightAbsorptionCoeff = 5.0f;
    float3 lightDir = float3(0.0f, 1.0f, 0.0f);
    float densityAbsorption = 10.0f;
    float3 lightColor = float3(1.0f, 1.0f, 1.0f) * 40.0f;
    float aniso = 0.3f;
}

// �ڽ� �����ڸ� ��ǥ�κ��� 3D �ؽ��� ��ǥ ���
float3 GetUVW(float3 posModel)
{
    return (posModel.xyz + 1.0f) * 0.5f;
}

// https://wallisc.github.io/rendering/2020/05/02/Volumetric-Rendering-Part-2.html
float BeerLambert(float absorptionCoefficient, float distanceTraveled)
{
    return exp(-absorptionCoefficient * distanceTraveled);
}

// Henyey-Greenstein phase function
// Graph: https://www.researchgate.net/figure/Henyey-Greenstein-phase-function-as-a-function-of-O-O-for-isotropic-scattering-g_fig1_338086693
float HenyeyGreensteinPhase(in float3 L, in float3 V, in float aniso)
{
    // V: eye - pos 
    // L: ������ ���ϴ� ����
    // https://www.shadertoy.com/view/7s3SRH
    
    float cosT = dot(L, -V);
    float g = aniso;
    return (1.0f - g * g) / (4.0f * PI * pow(abs(1.0f + g * g - 2.0f * g * cosT), 3.0f / 2.0f));
}

// https://github.com/maruel/temperature/blob/master/temperature.go
float3 ToRGB(float kelvin)
{
    if (kelvin == 6500.0f)
    {
        return float3(1.0f, 1.0f, 1.0f);
    }

    float temperature = kelvin * 0.01f;
    if (kelvin < 6500.0f)
    {
        float b = 0.0f;
        float r = 1.0f;
        float green = temperature - 2.0f;
        float g = (-155.25485562709179f - 0.44596950469579133f * green + 104.49216199393888f * log(green));

        if (kelvin > 2000.0f)
        {
            float blue = temperature - 10.0f;
            b = (-254.76935184120902f + 0.8274096064007395f * blue + 115.67994401066147f * log(blue)) * 255.0f;
        }
        
        return float3(r, g, b);
    }

    float b = 1.0f;
    float red = temperature - 55.0f;
    float r = (351.97690566805693f + 0.114206453784165f * red - 40.25366309332127f * log(red));
    float green = temperature - 50.0f;
    float g = (325.4494125711974f + 0.07943456536662342f * green - 28.0852963507957f * log(green)) * 255.0f;
    return float3(r, g, b);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    // ���� �ڽ� ��ǥ�迡�� ī�޶� ��ġ ���.
    float3 eyeModel = mul(float4(eyeWorld, 1), worldInv).xyz; // ����->�� ����ȯ
    float3 dirModel = normalize(input.posModel - eyeModel);
    
    int numSteps = 128;
    float stepSize = 2.0f / float(numSteps); // �ڽ� ���̰� 2.0
    
    // float absorptionCoeff = 10.0;
    float3 volumeAlbedo = float3(1.0f, 1.0f, 1.0f); // ���� ��.
    // float3 lightColor = float3(1, 1, 1) * 40.0;
    
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f); // visibility 1.0���� ����
    float3 posModel = input.posModel + dirModel * 1e-6; // ��¦ �� ���¿��� ����

    // ����: color.a�� "����"�� ����ϴٰ� �������� "������"�� �ٲ���
    
    [loop] // [unroll] ��� �� ���̴� ������ �ʹ� ����.
    for (int i = 0; i < numSteps; ++i)
    {
        float3 uvw = GetUVW(posModel); // +uvwOffset; �̻�� 

        // ��ü ������
        {
            float3 objCenter = float3(0.15f, 0.3f, 0.5f);
            float objRadius = 0.06;
            float dist = length((uvw - objCenter) * float3(2.0f, 1.0f, 1.0f)) / objRadius;
    
            if (dist < 1.0)
            {
                color.rgb += float3(0.0f, 0.0f, 1.0f) * color.a; // Blue ball
                color.a = 0.0f;
            
                // ����: ��ü���� ������ȭ�� ���� ������ �ϰ� ���̸� �����ؼ� ������ ���� �ֽ��ϴ�.
                break;
            }
        }
        
        float density = densityTex.SampleLevel(linearClampSampler, uvw, 0).r;
        // float lighting = lightingTex.SampleLevel(linearClampSampler, uvw, 0).r;
        float lighting = 1.0f; // ����Ʈ���� ���� ����
        
        // signed distance function.
        // float sdf = length(posModel) - 0.5f; // explicit surface.
        //float sdf1 = length(posModel + float3(-0.2f, 0.0f, 0.0f)) - 0.3f;
        //float sdf2 = length(posModel + float3(0.2f, 0.0f, 0.0f)) - 0.3f;
        //float sdf = min(sdf1, sdf2); // implicit surface.
        
        //if (sdf > 0.0f)
        //{
        //    density *= saturate(1.0f - sdf * 10.0f);
        //}

        if (density.r > 1e-3)
        {
            float prevAlpha = color.a; // ���İ� �ӽ� ����.
            color.a *= BeerLambert(densityAbsorption * density.r, stepSize); // ���� ����ϴ� ������ ����.
            float absorptionFromMarch = prevAlpha - color.a; // ����� ���� ���� ���� ���.
            
            color.rgb += absorptionFromMarch * volumeAlbedo * lightColor * density * lighting * 
                         HenyeyGreensteinPhase(lightDir, dirModel, aniso); // ���̿� ����ϵ��� ���� ���.
        }
        
        posModel += dirModel * stepSize;
        
        if (abs(posModel.x) > 1.0f || abs(posModel.y) > 1.0f || abs(posModel.z) > 1.0f) // ���� �ڽ� ����� ����.
        {
            break;
        }
        if (color.a < 1e-3) // ���� ����.
        {
            break;
        }
    }

    color = saturate(color);
    color.a = 1.0f - color.a; // a�� ������
    // skybox�� �̹� ������ �Ǿ� �ִ� ����. ���� ���� �ڽ��� ���� ���� �ǹ̰� ����.
    
    return color;
}
