cbuffer cbPerObject : register(b2) {
    matrix LocalMatrix : packoffset(c0);
    float3 ShadeColor : packoffset(c4.x); // for non-mesh objects that use a colour (beams/null models/etc)
    float AlphaVal : packoffset(c4.w);
};

struct PS_NULL {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

PS_NULL GetNullVert(float3 Position, float3 Normal)
{
    PS_NULL gs_out;

    gs_out.Position = mul(LocalMatrix, float4 (Position * 16.0f, 1));
    gs_out.Normal = Normal;

    return gs_out;
}

[maxvertexcount(3)]
void GeometryShaderEntryPoint(triangle float3 gs_in[3] : POSITION, inout TriangleStream<PS_NULL> gs_out)
{
    // this is the same normal calculation as QBSP does
    float3 Normal = normalize(cross(gs_in[0] - gs_in[1], gs_in[2] - gs_in[1]));

    gs_out.Append(GetNullVert(gs_in[0], Normal));
    gs_out.Append(GetNullVert(gs_in[1], Normal));
    gs_out.Append(GetNullVert(gs_in[2], Normal));
}