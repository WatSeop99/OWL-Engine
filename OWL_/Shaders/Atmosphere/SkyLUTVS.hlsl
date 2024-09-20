struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

PixelShaderInput main(uint vertexID : SV_VertexID)
{
    PixelShaderInput output;
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.5f, 1.0f);
    return output;
}