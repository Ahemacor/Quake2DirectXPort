cbuffer PerFrameConstants : register (b0)
{
    float scale;
}


struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexShaderOutput VertexShaderEntryPoint(float4 position : POSITION, float2 uv : TEXCOORD, uint vertexId : SV_VertexID)
{
    VertexShaderOutput output;
    output.position = position;
    output.position.xy *= scale;
    output.uv = uv;
    //output.position = float4((float)(vertexId / 2) * 4.0f - 1.0f, (float)(vertexId % 2) * 4.0f - 1.0f, 0, 1);
    //output.uv = float2 ((float)(vertexId / 2) * 2.0f, 1.0f - (float)(vertexId % 2) * 2.0f);
    return output;
}