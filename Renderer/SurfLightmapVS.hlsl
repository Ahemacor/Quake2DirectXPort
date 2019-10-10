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

Buffer<float> LightStyles : register(t7);

struct PS_BASIC {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VS_SURFCOMMON {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float2 Lightmap : LIGHTMAP;
    uint4 Styles: STYLES;
    uint MapNum : MAPNUM;
    float Scroll : SCROLL;
};

struct PS_LIGHTMAPPED {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Lightmap : LIGHTMAP;
    nointerpolation float4 Styles: STYLES;
};

float2 GetTextureScroll(VS_SURFCOMMON vs_in)
{
    // ensure that SURF_FLOWING scroll is applied consistently
    return vs_in.TexCoord + float2 (TexScroll, 0.0f) * vs_in.Scroll;
}

PS_LIGHTMAPPED SurfLightmapVS(VS_SURFCOMMON vs_in)
{
    PS_LIGHTMAPPED vs_out;

    vs_out.Position = mul(LocalMatrix, vs_in.Position);
    vs_out.TexCoord = GetTextureScroll(vs_in);
    vs_out.Lightmap = float3 (vs_in.Lightmap, vs_in.MapNum);

    vs_out.Styles = float4 (
        LightStyles.Load(vs_in.Styles.x),
        LightStyles.Load(vs_in.Styles.y),
        LightStyles.Load(vs_in.Styles.z),
        LightStyles.Load(vs_in.Styles.w)
        );

    return vs_out;
}