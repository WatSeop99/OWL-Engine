struct PixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

cbuffer ShadowConstants : register(b0)
{
    matrix g_LightViewProj[6];
};

[maxvertexcount(18)]
void main(triangle float4 InPos[3] : SV_POSITION, inout TriangleStream<PixelShaderInput> OutStream)
{
    for (int face = 0; face < 6; ++face)
    {
        PixelShaderInput output;
        output.RTIndex = face;

        for (int v = 0; v < 3; ++v)
        {
            output.ProjectedPosition = mul(InPos[v], g_LightViewProj[face]);
            OutStream.Append(output);
        }
        
        OutStream.RestartStrip();
    }
}
