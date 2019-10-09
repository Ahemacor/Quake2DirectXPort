cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

Texture2D<float4> mainTexture : register(t0);

sampler cineSampler : register(s4);

struct PS_DRAWTEXTURED {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 TexCoord : TEXCOORD;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 DrawCinematicPS(PS_DRAWTEXTURED ps_in) : SV_TARGET0
{
    return GetGamma(mainTexture.Sample(cineSampler, ps_in.TexCoord));
}