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

struct GS_PARTICLE {
    float3 Origin : ORIGIN;
    float4 Color : COLOR;
};

struct PS_PARTICLE {
    float4 Position : SV_POSITION;
    float4 Color : COLOUR;
    float2 Offsets : OFFSETS;
};

PS_PARTICLE GetParticleVert(point GS_PARTICLE gs_in, float2 Offsets, float ScaleUp)
{
    PS_PARTICLE gs_out;

    // compute new particle origin
    float3 Position = gs_in.Origin + (viewRight * Offsets.x + viewUp * Offsets.y) * ScaleUp;

    // and write it out
    gs_out.Position = mul(mvpMatrix, float4 (Position, 1.0f));
    gs_out.Color = gs_in.Color;
    gs_out.Offsets = Offsets;

    return gs_out;
}

void ParticleCommonGS(point GS_PARTICLE gs_in, inout TriangleStream<PS_PARTICLE> gs_out, float TypeScale, float HackUp)
{
    // frustum cull the particle
    if (dot(gs_in.Origin, frustum0.xyz) - frustum0.w <= -1) return;
    if (dot(gs_in.Origin, frustum1.xyz) - frustum1.w <= -1) return;
    if (dot(gs_in.Origin, frustum2.xyz) - frustum2.w <= -1) return;
    if (dot(gs_in.Origin, frustum3.xyz) - frustum3.w <= -1) return;

    // hack a scale up to keep particles from disapearing
    float ScaleUp = (1.0f + dot(gs_in.Origin - viewOrigin, viewForward) * HackUp) * TypeScale;

    // and write it out
    gs_out.Append(GetParticleVert(gs_in, float2 (-1, -1), ScaleUp));
    gs_out.Append(GetParticleVert(gs_in, float2 (-1, 1), ScaleUp));
    gs_out.Append(GetParticleVert(gs_in, float2 (1, -1), ScaleUp));
    gs_out.Append(GetParticleVert(gs_in, float2 (1, 1), ScaleUp));
}

[maxvertexcount(4)]
void ParticleSquareGS(point GS_PARTICLE gs_in[1], inout TriangleStream<PS_PARTICLE> gs_out)
{
    ParticleCommonGS(gs_in[0], gs_out, 0.5f, 0.002f);
}