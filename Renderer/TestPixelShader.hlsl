Texture2D<float4> anteruTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PixelShaderEntryPoint(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET
{
    return anteruTexture.Sample(texureSampler, uv);
}