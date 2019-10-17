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

cbuffer cbPerLight : register(b4) {
    float3 LightOrigin : packoffset(c0.x);
    float LightRadius : packoffset(c0.w);
    float3 LightColour : packoffset(c1.x);
};

Texture2D<float4> mainTexture[1024] : register(t0);

sampler mainSampler : register(s0);

// common to mesh and surf
struct PS_DYNAMICLIGHT {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 LightVector : LIGHTVECTOR;
    float3 Normal : NORMAL;
    uint TextureId : TEXTURE;
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

float4 GenericDynamicPS(PS_DYNAMICLIGHT ps_in) : SV_TARGET0
{
    // this clip is sufficient to exclude unlit portions; Add below may still bring it to 0
    // but in practice it's rare and it runs faster without a second clip
    clip((LightRadius * LightRadius) - dot(ps_in.LightVector, ps_in.LightVector));

    // reading the diffuse texture early so that it should interleave with some ALU ops
    float4 diff = GetGamma(mainTexture[ps_in.TextureId].Sample(mainSampler, ps_in.TexCoord));

    // this calc isn't correct per-theory but it matches with the calc used by light.exe and qrad.exe
    // at this stage we don't adjust for the overbright range; that will be done via the "intensity" cvar in the C code
    float Angle = ((dot(normalize(ps_in.Normal), normalize(ps_in.LightVector)) * 0.5f) + 0.5f) / 256.0f;

    // using our own custom attenuation, again it's not correct per-theory but matches the Quake tools
    float Add = max((LightRadius - length(ps_in.LightVector)) * Angle, 0.0f);

    return float4 (diff.rgb * Desaturate(LightColour) * Add, 0.0f);
}