cbuffer cbPerObject : register(b2) {
    matrix LocalMatrix : packoffset(c0);
    float3 ShadeColor : packoffset(c4.x); // for non-mesh objects that use a colour (beams/null models/etc)
    float AlphaVal : packoffset(c4.w);
};

cbuffer cbPerMesh : register(b3) {
    float3 ShadeLight : packoffset(c0);
    float3 ShadeVector : packoffset(c1);
    float3 Move : packoffset(c2);
    float3 FrontV : packoffset(c3);
    float3 BackV : packoffset(c4);
    float PowersuitScale : packoffset(c5.x);
    float BackLerp : packoffset(c5.y);
};

Buffer<float4> LightNormals : register(t8);

struct VS_MESH {
    uint4 PrevTriVertx: PREVTRIVERTX;
    uint4 CurrTriVertx: CURRTRIVERTX;
    float2 TexCoord: TEXCOORD;
};

struct PS_MESH {
    float4 Position: SV_POSITION;
    float2 TexCoord: TEXCOORD;
    float3 Normal : NORMAL;
};

float4 MeshLerpPosition(VS_MESH vs_in)
{
    return float4 (Move + vs_in.CurrTriVertx.xyz * FrontV + vs_in.PrevTriVertx.xyz * BackV, 1.0f);
}

float3 MeshLerpNormal(VS_MESH vs_in)
{
    // note: this is the correct order for normals; check the light on the hyperblaster v_ model, for example;
    // with the opposite order it flickers pretty badly as the model animates; with this order it's nice and solid
    float3 n1 = LightNormals.Load(vs_in.CurrTriVertx.w).xyz;
    float3 n2 = LightNormals.Load(vs_in.PrevTriVertx.w).xyz;
    return normalize(lerp(n1, n2, BackLerp));
}

PS_MESH MeshLightmapVS(VS_MESH vs_in)
{
    PS_MESH vs_out;

    vs_out.Position = mul(LocalMatrix, MeshLerpPosition(vs_in));
    vs_out.TexCoord = vs_in.TexCoord;
    vs_out.Normal = MeshLerpNormal(vs_in);

    return vs_out;
}