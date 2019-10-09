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

Texture2D<float4> mainTexture : register(t0);

sampler mainSampler : register(s0);

struct PS_DRAWTURB {
    float4 Position : SV_POSITION;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

float4 GetGamma(float4 colorin)
{
    return float4 (pow(max(colorin.rgb * v_contrast, 0.0f), v_gamma), colorin.a);
}

float4 SurfDrawTurbPS(PS_DRAWTURB ps_in) : SV_TARGET0
{
    // original GLQuakeII scaled down the warp by 0.5 but that was just a hack to prevent breakup from per-vertex warps; we treat software as the reference
    float4 diff = GetGamma(mainTexture.SampleGrad(mainSampler, ps_in.TexCoord0 + sin(ps_in.TexCoord1) * 0.125f, ddx(ps_in.TexCoord0), ddy(ps_in.TexCoord0)));
    //float4 diff = GetGamma (mainTexture.SampleGrad (mainSampler, ps_in.TexCoord0 + sin (ps_in.TexCoord1) * 0.0625f, ddx (ps_in.TexCoord0), ddy (ps_in.TexCoord0)));
    return float4 (diff.rgb, diff.a * AlphaVal);
}