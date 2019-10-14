cbuffer cbPerObject : register(b2) {
    matrix LocalMatrix : packoffset(c0);
    float3 ShadeColor : packoffset(c4.x); // for non-mesh objects that use a colour (beams/null models/etc)
    float AlphaVal : packoffset(c4.w);
};

struct LightNormal
{
    float4 normal[162];
};
ConstantBuffer<LightNormal> lNormals : register(b7);

cbuffer cbPerMesh : register(b3) {
    float3 ShadeLight : packoffset(c0);
    float3 ShadeVector : packoffset(c1);
    float3 Move : packoffset(c2);
    float3 FrontV : packoffset(c3);
    float3 BackV : packoffset(c4);
    float PowersuitScale : packoffset(c5.x);
    float BackLerp : packoffset(c5.y);
};

struct VS_MESH {
    uint4 PrevTriVertx: PREVTRIVERTX;
    uint4 CurrTriVertx: CURRTRIVERTX;
    float2 TexCoord: TEXCOORD;
};

float4 MeshLerpPosition(VS_MESH vs_in)
{
    return float4 (Move + vs_in.CurrTriVertx.xyz * FrontV + vs_in.PrevTriVertx.xyz * BackV, 1.0f);
}

float3 MeshLerpNormal(VS_MESH vs_in)
{
    // note: this is the correct order for normals; check the light on the hyperblaster v_ model, for example;
    // with the opposite order it flickers pretty badly as the model animates; with this order it's nice and solid
    float3 n1 = lNormals.normal[vs_in.CurrTriVertx.w].xyz;
    float3 n2 = lNormals.normal[vs_in.PrevTriVertx.w].xyz;
    return normalize(lerp(n1, n2, BackLerp));
}

float4 MeshPowersuitVS(VS_MESH vs_in) : SV_POSITION
{
    return mul(LocalMatrix, MeshLerpPosition(vs_in) + float4 (MeshLerpNormal(vs_in) * PowersuitScale, 0.0f));
}