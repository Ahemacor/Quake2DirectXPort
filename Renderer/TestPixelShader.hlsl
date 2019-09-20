struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PixelShaderEntryPoint(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET
{
    //return float4(uv, 0, 1);
    return float4(1,1,1,1);
}