struct PS_BASIC {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 SkyNoSkyPS(PS_BASIC ps_in) : SV_TARGET0
{
    return float4 (1, 1, 1, 1);
}