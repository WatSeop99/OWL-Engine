static const float2 TEXCOORD_DATA[6] =
{
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f)
};
static const float4 POSITION_DATA[6] =
{
    float4(-1.0f, 1.0f, 0.0f, 1.0f),
    float4(1.0f, 1.0f, 0.0f, 1.0f),
    float4(1.0f, -1.0f, 0.0f, 1.0f),
    float4(-1.0f, 1.0f, 0.0f, 1.0f),
    float4(1.0f, -1.0f, 0.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
};

struct PixelShaderInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

PixelShaderInput main(uint vertexID : SV_VertexID)
{
    PixelShaderInput output;
    output.TexCoord = TEXCOORD_DATA[vertexID];
    output.Position = POSITION_DATA[vertexID];

    return output;
}
