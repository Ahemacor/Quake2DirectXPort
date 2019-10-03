float4 VertexShaderEntryPoint(uint vertexId : SV_VertexID) : SV_POSITION
{
    return float4 ((float)(vertexId / 2) * 4.0f - 1.0f, (float)(vertexId % 2) * 4.0f - 1.0f, 0, 1);
}