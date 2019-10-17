struct VS_SURFCOMMON {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float2 Lightmap : LIGHTMAP;
    uint4 Styles: STYLES;
    uint MapNum : MAPNUM;
    float Scroll : SCROLL;
    uint TextureId : TEXTURE;
};

VS_SURFCOMMON SurfDynamicVS(VS_SURFCOMMON vs_in)
{
    return vs_in;
}