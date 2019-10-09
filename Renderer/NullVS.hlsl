static const uint NULLIndexes[24] = { 1, 4, 2, 1, 2, 5, 1, 5, 3, 1, 3, 4, 0, 4, 3, 0, 3, 5, 0, 5, 2, 0, 2, 4 };

static const float3 NULLPositions[6] = {
    float3 (0.0f, 0.0f, 1.0f),
    float3 (0.0f, 0.0f, -1.0f),
    float3 (0.0f, 1.0f, 0.0f),
    float3 (0.0f, -1.0f, 0.0f),
    float3 (1.0f, 0.0f, 0.0f),
    float3 (-1.0f, 0.0f, 0.0f)
};

float3 NullVS(uint vertexId : SV_VertexID) : POSITION
{
    return NULLPositions[NULLIndexes[vertexId]];
}