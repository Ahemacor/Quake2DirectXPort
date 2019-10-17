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

cbuffer cbPerLight : register(b4) {
    float3 LightOrigin : packoffset(c0.x);
    float LightRadius : packoffset(c0.w);
    float3 LightColour : packoffset(c1.x);
};

struct VS_SURFCOMMON {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float2 Lightmap : LIGHTMAP;
    uint4 Styles: STYLES;
    uint MapNum : MAPNUM;
    float Scroll : SCROLL;
    uint TextureId : TEXTURE;
};

struct PS_DYNAMICLIGHT {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 LightVector : LIGHTVECTOR;
    float3 Normal : NORMAL;
    uint TextureId : TEXTURE;
};

// this ensures that we use the same dynamic calcs on surfs and meshs
// outside of the #ifdef guard because it's used by the VS for meshs and the GS for surfs
PS_DYNAMICLIGHT GenericDynamicVS(float4 Position, float3 Normal, float2 TexCoord, uint textureId)
{
    PS_DYNAMICLIGHT vs_out;

    vs_out.Position = mul(LocalMatrix, Position);
    vs_out.TexCoord = TexCoord;
    vs_out.LightVector = LightOrigin - Position.xyz;
    vs_out.Normal = Normal;
    vs_out.TextureId = textureId;

    return vs_out;
}

float2 GetTextureScroll(VS_SURFCOMMON vs_in)
{
    // ensure that SURF_FLOWING scroll is applied consistently
    return vs_in.TexCoord + float2 (TexScroll, 0.0f) * vs_in.Scroll;
}

[maxvertexcount(3)]
void SurfDynamicGS(triangle VS_SURFCOMMON gs_in[3], inout TriangleStream<PS_DYNAMICLIGHT> gs_out)
{
    // this is the same normal calculation as QBSP does
    float3 Normal = normalize(
        cross(gs_in[0].Position.xyz - gs_in[1].Position.xyz, gs_in[2].Position.xyz - gs_in[1].Position.xyz)
    );

    // output position needs to use the same transform as the prior pass to satisfy invariance rules
    // we inverse-transformed the light position by the entity local matrix so we don't need to transform the normal or the light vector stuff
    gs_out.Append(GenericDynamicVS(gs_in[0].Position, Normal, GetTextureScroll(gs_in[0]), gs_in[0].TextureId));
    gs_out.Append(GenericDynamicVS(gs_in[1].Position, Normal, GetTextureScroll(gs_in[1]), gs_in[1].TextureId));
    gs_out.Append(GenericDynamicVS(gs_in[2].Position, Normal, GetTextureScroll(gs_in[2]), gs_in[2].TextureId));
}