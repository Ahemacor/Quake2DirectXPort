cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

cbuffer cbMainPerFrame : register(b1) {
    matrix mvpMatrix : packoffset(c0);
    float3 viewOrigin : packoffset(c4);
    float3 viewForward : packoffset(c5);
    float3 viewRight : packoffset(c6);
    float3 viewUp : packoffset(c7);
    float4 vBlend : packoffset(c8);
    float2 WarpTime : packoffset(c9.x);
    float TexScroll : packoffset(c9.z);
    float turbTime : packoffset(c9.w);

    // in case we ever need these...
    float RefdefX : packoffset(c10.x);
    float RefdefY : packoffset(c10.y);
    float RefdefW : packoffset(c10.z);
    float RefdefH : packoffset(c10.w);

    matrix SkyMatrix : packoffset(c11);

    float4 frustum0 : packoffset(c15);
    float4 frustum1 : packoffset(c16);
    float4 frustum2 : packoffset(c17);
    float4 frustum3 : packoffset(c18);

    float desaturation : packoffset(c19.x);
};

cbuffer cbPerObject : register(b2) {
    matrix LocalMatrix : packoffset(c0);
    float3 ShadeColor : packoffset(c4.x); // for non-mesh objects that use a colour (beams/null models/etc)
    float AlphaVal : packoffset(c4.w);
};

Texture2D<float4> mainTexture : register(t0);
Texture2DArray<float4> lmap0Texture : register(t10);	// lightmap styles 0/1/2/3
Texture2DArray<float4> lmap1Texture : register(t11);	// lightmap styles 0/1/2/3
Texture2DArray<float4> lmap2Texture : register(t12);	// lightmap styles 0/1/2/3

sampler mainSampler : register(s0);
sampler lmapSampler : register(s1);

struct PS_BASIC {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

struct PS_LIGHTMAPPED {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Lightmap : LIGHTMAP;
    nointerpolation float4 Styles: STYLES;
};

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3 (R, G, B));
}

float3 HSLtoRGB(in float3 HSL)
{
    float3 RGB = HUEtoRGB(HSL.x);
    float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
    return (RGB - 0.5) * C + HSL.z;
}

float3 RGBtoHCV(in float3 RGB)
{
    float Epsilon = 1e-10;
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4 (RGB.bg, -1.0, 2.0 / 3.0) : float4 (RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4 (P.xyw, RGB.r) : float4 (RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3 (H, C, Q.x);
}

float3 RGBtoHSL(in float3 RGB)
{
    float Epsilon = 1e-10;
    float3 HCV = RGBtoHCV(RGB);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
    return float3 (HCV.x, S, L);
}

float3 Desaturate(float3 RGB)
{
    // hack hack hack - because light can overbright we scale it down, then apply the desaturation, then bring it back up again
    // otherwise we get clamping issues in the conversion funcs if any of the channels are above 1
    return HSLtoRGB(RGBtoHSL(RGB * 0.1f) * float3 (1.0f, desaturation, 1.0f)) * 10.0f;
}

float4 SurfLightmapPS(PS_LIGHTMAPPED ps_in) : SV_TARGET0
{
    float4 diff = GetGamma(mainTexture.Sample(mainSampler, ps_in.TexCoord));

    float3 lmap = float3 (
        dot(lmap0Texture.Sample(lmapSampler, ps_in.Lightmap), ps_in.Styles),
        dot(lmap1Texture.Sample(lmapSampler, ps_in.Lightmap), ps_in.Styles),
        dot(lmap2Texture.Sample(lmapSampler, ps_in.Lightmap), ps_in.Styles)
    );

    //return float4 (diff.rgb * Desaturate(lmap), 1.0f);

    return float4 (diff.rgb * max(0.1f, Desaturate(lmap)), 1.0f);
}