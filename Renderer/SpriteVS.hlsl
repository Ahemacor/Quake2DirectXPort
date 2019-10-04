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

struct PS_SPRITE {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

static const float2 SpriteTexCoords[4] = { float2 (0, 1), float2 (0, 0), float2 (1, 0), float2 (1, 1) };

PS_SPRITE VertexShaderEntryPoint(float2 XYOffset : XYOFFSET, uint vertexId : SV_VertexID)
{
    PS_SPRITE vs_out;

    vs_out.Position = mul(LocalMatrix, float4 ((viewRight * XYOffset.y) + (viewUp * XYOffset.x), 1.0f));
    vs_out.TexCoord = SpriteTexCoords[vertexId];

    return vs_out;
}