#include "Common.hlsli" // ���̴������� include ��� ����

struct PixelShaderOutput
{
    float4 pixelColor : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    output.pixelColor.rgba = 1.0f;
    
    return output;
}
