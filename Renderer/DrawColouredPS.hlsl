cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

struct PS_DRAWCOLOURED {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 DrawColouredPS(PS_DRAWCOLOURED ps_in) : SV_TARGET0
{
    // this is a quad filled with a single solid colour so it doesn't need to blend
    return GetGamma(ps_in.Color);
}