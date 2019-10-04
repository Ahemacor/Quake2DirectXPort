cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

Texture2DArray<float4> charTexture : register(t6);	// characters and numbers

sampler drawSampler : register(s3);

struct PS_DRAWCHARACTER {
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 PixelShaderEntryPoint(PS_DRAWCHARACTER ps_in) : SV_TARGET0
{
    float4 diff = GetGamma(charTexture.Sample(drawSampler, ps_in.TexCoord));
    return float4 (diff.rgb * diff.a, diff.a);
}