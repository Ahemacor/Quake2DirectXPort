cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

Texture2D<float4> mainTexture[1024] : register(t0);	// main diffuse texture on most objects

sampler drawSampler : register(s3);		// used for the 2d render; linear sampled, clamp mode, no mips, no anisotropy

struct PS_DRAWTEXTURED {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 TexCoord : TEXCOORD;
    uint TextureId : TEXTURE;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 DrawTexturedPS(PS_DRAWTEXTURED ps_in) : SV_TARGET
{
    float4 diff = GetGamma(mainTexture[ps_in.TextureId].Sample(drawSampler, ps_in.TexCoord)) * ps_in.Color;
    return float4 (diff.rgb * diff.a, diff.a);
}