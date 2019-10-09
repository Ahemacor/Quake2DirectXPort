cbuffer cbDrawPerFrame : register(b0) {
    matrix orthoMatrix : packoffset(c0);
    float v_gamma : packoffset(c4.x);
    float v_contrast : packoffset(c4.y);
    float2 ConScale : packoffset(c4.z);
};

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

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 MeshPowersuitPS(float4 Position: SV_POSITION) : SV_TARGET0
{
    return GetGamma(float4 (ShadeLight, AlphaVal));
}