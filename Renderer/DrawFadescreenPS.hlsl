float4 DrawFadescreenPS(float4 Position : SV_POSITION) : SV_TARGET0
{
    // no gamma needed because rgb is black
    return float4 (0, 0, 0, 0.666f);
}
