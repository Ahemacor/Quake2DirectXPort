cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

struct VS_DRAWCOMMON {
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOUR;
};

struct PS_DRAWTEXTURED {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 TexCoord : TEXCOORD;
};

PS_DRAWTEXTURED DrawCinematicVS(VS_DRAWCOMMON vs_in)
{
    PS_DRAWTEXTURED vs_out;

    vs_out.Position = vs_in.Position;
    vs_out.Color = vs_in.Color;
    vs_out.TexCoord = vs_in.Position.xy * vs_in.TexCoord + 0.5f;

    return vs_out;
}