cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

struct VS_DRAWCHARACTER {
    float4 Position : POSITION;
    float3 TexCoord : TEXCOORD;
};

struct PS_DRAWCHARACTER {
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

PS_DRAWCHARACTER VertexShaderEntryPoint(VS_DRAWCHARACTER vs_in)
{
    PS_DRAWCHARACTER vs_out;

    vs_out.Position = mul(orthoMatrix, vs_in.Position);
    vs_out.TexCoord = vs_in.TexCoord;

    return vs_out;
}