cbuffer PerFrameConstants : register (b0)
{
    float scale;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexShaderOutput VertexShaderEntryPoint(float4 position : POSITION, float2 uv : TEXCOORD)
{
    VertexShaderOutput output;

    output.position = position;
    output.position.xy *= scale;
    output.uv = uv;

    return output;
}