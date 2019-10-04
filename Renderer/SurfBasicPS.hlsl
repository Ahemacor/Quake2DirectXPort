cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

Texture2D<float4> mainTexture : register(t0);	// main diffuse texture on most objects

sampler mainSampler : register(s0);		// used for the 2d render; linear sampled, clamp mode, no mips, no anisotropy

struct PS_BASIC {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

struct PS_DRAWTEXTURED {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 TexCoord : TEXCOORD;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 PixelShaderEntryPoint(PS_BASIC ps_in) : SV_TARGET0
{
    float4 diff = GetGamma(mainTexture.Sample(mainSampler, ps_in.TexCoord));
    return float4 (diff.rgb, 1.0f);
}