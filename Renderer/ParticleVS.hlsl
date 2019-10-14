struct Palette
{
    float3 color[256];
};

ConstantBuffer<Palette> qPalette : register(b5);

struct VS_PARTICLE {
    float3 Origin : ORIGIN;
    float3 Velocity : VELOCITY;
    float3 Acceleration : ACCELERATION;
    float Time : TIME;
    int Color : COLOR;
    float Alpha : ALPHA;
};

struct GS_PARTICLE {
    float3 Origin : ORIGIN;
    float4 Color : COLOR;
};

GS_PARTICLE ParticleVS(VS_PARTICLE vs_in)
{
    GS_PARTICLE vs_out;

    // move the particle in a framerate-independent manner
    vs_out.Origin = vs_in.Origin + (vs_in.Velocity + vs_in.Acceleration * vs_in.Time) * vs_in.Time;

    vs_out.Color = float4 (qPalette.color[vs_in.Color], vs_in.Alpha);

    return vs_out;
}