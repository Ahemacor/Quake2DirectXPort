struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexShaderOutput VertexShaderEntryPoint(float4 position : POSITION, float2 uv : TEXCOORD)
{
    VertexShaderOutput output;

    output.position = position;
    output.uv = uv;

    return output;
}