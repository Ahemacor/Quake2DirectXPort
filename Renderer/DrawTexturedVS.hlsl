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

PS_DRAWTEXTURED VertexShaderEntryPoint (VS_DRAWCOMMON vs_in, uint vertexId : SV_VertexID)
{
    PS_DRAWTEXTURED vs_out;

    vs_out.Position = mul(orthoMatrix, vs_in.Position);
    vs_out.Color = vs_in.Color;
    vs_out.TexCoord = vs_in.TexCoord;

    /*
    if (vertexId == 0) vs_out.Position = float4(-0.5f, 0.5f, 0, 1); // upper left
    else if (vertexId == 1) vs_out.Position = float4(0.5f, 0.5f, 0, 1); // upper right
    else if (vertexId == 2) vs_out.Position = float4(0.5f, -0.5f, 0, 1); // bottom right

    else if (vertexId == 3) vs_out.Position = float4(-0.5f, 0.5f, 0, 1); // upper left
    else if (vertexId == 4) vs_out.Position = float4(0.5f, -0.5f, 0, 1); // bottom right
    else if (vertexId == 5) vs_out.Position = float4(-0.5f, -0.5f, 0, 1); // bottom left
    */

    return vs_out;
}