cbuffer cbPerObject : register(b2) {
    matrix LocalMatrix : packoffset(c0);
    float3 ShadeColor : packoffset(c4.x);
    float AlphaVal : packoffset(c4.w);
};

float4 BeamVS(float4 Position: POSITION) : SV_POSITION
{
    return mul(LocalMatrix, Position);
}