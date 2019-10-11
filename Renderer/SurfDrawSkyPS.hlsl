cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

TextureCube<float4> sboxTexture : register(t4);

sampler lmapSampler : register(s1);

struct PS_DRAWSKY {
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

float4 GetGamma(float4 colorin)
{
    // gamma is not applied to alpha
    // this isn't actually "contrast" but it's consistent with what other engines do
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 SurfDrawSkyPS(PS_DRAWSKY ps_in) : SV_TARGET0
{
    // reuses the lightmap sampler because it has the same sampler state as is required here
    return GetGamma(sboxTexture.Sample(lmapSampler, ps_in.TexCoord));
}