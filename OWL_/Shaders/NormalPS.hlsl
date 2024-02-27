struct PixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    float3 Color : COLOR;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(input.Color, 1.0f);
}