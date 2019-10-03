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

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

struct PS_NULL {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

float4 PixelShaderEntryPoint(PS_NULL ps_in) : SV_TARGET0
{
    float shadedot = dot(normalize(ps_in.Normal), normalize(float3 (1.0f, 1.0f, 1.0f)));
    return GetGamma(float4 (ShadeColor * max(shadedot + 1.0f, (shadedot * 0.2954545f) + 1.0f), AlphaVal));
}