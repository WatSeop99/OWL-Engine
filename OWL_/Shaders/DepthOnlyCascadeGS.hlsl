cbuffer ShadowConstants : register(b0)
{
    matrix LightViewProj[6];
};

struct PixelShaderInput
{
    float4 ProjectedPosition : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(9)]
void main(triangle float4 InPos[3] : SV_POSITION, inout TriangleStream<PixelShaderInput> OutStream)
{
    for (int cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex)
    {
        PixelShaderInput output;
        output.RTIndex = cascadeIndex;

        for (int v = 0; v < 3; ++v)
        {
            output.ProjectedPosition = mul(InPos[v], LightViewProj[cascadeIndex]);
            OutStream.Append(output);
        }
        
        OutStream.RestartStrip();
    }
}
