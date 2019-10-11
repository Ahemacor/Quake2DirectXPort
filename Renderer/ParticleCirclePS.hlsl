cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

float4 GetGamma(float4 colorin)
{
    // gamma is not applied to alpha
    // this isn't actually "contrast" but it's consistent with what other engines do
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

struct PS_PARTICLE {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 Offsets : OFFSETS;
};

float4 ParticleCirclePS(PS_PARTICLE ps_in) : SV_TARGET0
{
    // procedurally generate the particle dot for good speed and per-pixel accuracy at any scale
    float pAlpha = ps_in.Color.a * (1.0f - dot(ps_in.Offsets, ps_in.Offsets));
    clip(pAlpha); // reject any particles contributing less than zero
    return GetGamma(float4 (ps_in.Color.rgb * pAlpha, pAlpha));
}