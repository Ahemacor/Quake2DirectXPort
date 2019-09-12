#include "StateManager.h"
#include "CppWrapper.h"
#include <cassert>

ID3D11BlendState* StateManager::EnumSelectBlendState(BlendState bs)
{
    switch (bs)
    {
    case BlendState::BSNone:
        return BSNone.Get();
    case BlendState::BSAlphaBlend:
        return BSAlphaBlend.Get();
    case BlendState::BSAlphaReverse:
        return BSAlphaReverse.Get();
    case BlendState::BSAlphaPreMult:
        return BSAlphaPreMult.Get();
    case BlendState::BSAdditive:
        return BSAdditive.Get();
    case BlendState::BSRevSubtract:
        return BSRevSubtract.Get();
    default:
        assert(false);
    }
}

ID3D11DepthStencilState* StateManager::EnumSelectDepthStencilState(DepthStencilState ds)
{
    switch (ds)
    {
    case DepthStencilState::DSFullDepth:
        return DSFullDepth.Get();
    case DepthStencilState::DSDepthNoWrite:
        return DSDepthNoWrite.Get();
    case DepthStencilState::DSNoDepth:
        return DSNoDepth.Get();
    case DepthStencilState::DSEqualDepthNoWrite:
        return DSEqualDepthNoWrite.Get();
    default:
        assert(false);
    }
}

ID3D11RasterizerState* StateManager::EnumSelectRasterizerState(RasterizerState rs)
{
    switch (rs)
    {
    case RasterizerState::RSFullCull:
        return RSFullCull.Get();
    case RasterizerState::RSReverseCull:
        return RSReverseCull.Get();
    case RasterizerState::RSNoCull:
        return RSNoCull.Get();
    default:
        assert(false);
    }
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> StateManager::CreateSamplerState(D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressMode, float MaxLOD, UINT MaxAnisotropy)
{
    Microsoft::WRL::ComPtr<ID3D11SamplerState> ss;

    D3D11_SAMPLER_DESC desc;
    desc.AddressU = AddressMode;
    desc.AddressV = AddressMode;
    desc.AddressW = AddressMode;

    // border colour is always black because that's what our clamp-to-border code elsewhere in the engine expects
    desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0;

    // nope, not doing this
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    if (MaxAnisotropy > 1)
    {
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.MaxAnisotropy = MaxAnisotropy;
    }
    else
    {
        desc.Filter = Filter;
        desc.MaxAnisotropy = 1;
    }

    desc.MaxLOD = MaxLOD;
    desc.MinLOD = 0;
    desc.MipLODBias = 0;

    RWGetDevice()->CreateSamplerState(&desc, ss.GetAddressOf());

    return ss;
}

Microsoft::WRL::ComPtr<ID3D11BlendState> StateManager::CreateBlendState(BOOL blendon, D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op)
{
    Microsoft::WRL::ComPtr<ID3D11BlendState> bs;

    D3D11_BLEND_DESC desc;
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;
    desc.RenderTarget[0].BlendEnable = blendon;
    desc.RenderTarget[0].SrcBlend = src;
    desc.RenderTarget[0].DestBlend = dst;
    desc.RenderTarget[0].BlendOp = op;
    desc.RenderTarget[0].SrcBlendAlpha = src;
    desc.RenderTarget[0].DestBlendAlpha = dst;
    desc.RenderTarget[0].BlendOpAlpha = op;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    RWGetDevice()->CreateBlendState( &desc, bs.GetAddressOf());

    return bs;
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilState> StateManager::CreateDepthState(BOOL test, BOOL mask, D3D11_COMPARISON_FUNC func)
{
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> ds;
    D3D11_DEPTH_STENCIL_DESC desc;

    if (test)
    {
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = mask ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    }
    else
    {
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    }

    desc.DepthFunc = func;
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    RWGetDevice()->CreateDepthStencilState(&desc, ds.GetAddressOf());

    return ds;
}

Microsoft::WRL::ComPtr<ID3D11RasterizerState> StateManager::CreateRasterizerState(D3D11_FILL_MODE fill, D3D11_CULL_MODE cull, BOOL clip, BOOL scissor)
{
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rs;

    D3D11_RASTERIZER_DESC desc;
    desc.FillMode = fill;
    desc.CullMode = cull;
    desc.FrontCounterClockwise = TRUE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0;
    desc.SlopeScaledDepthBias = 0;
    desc.DepthClipEnable = clip;
    desc.ScissorEnable = scissor;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    RWGetDevice()->CreateRasterizerState(&desc, rs.GetAddressOf());

    return rs;
}

void StateManager::SetDefaultState()
{
    BSNone = CreateBlendState(FALSE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);
    BSAlphaBlend = CreateBlendState(TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);
    BSAlphaReverse = CreateBlendState(TRUE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_OP_ADD);
    BSAlphaPreMult = CreateBlendState(TRUE, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD);
    BSAdditive = CreateBlendState(TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD);
    BSRevSubtract = CreateBlendState(TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_REV_SUBTRACT);

    DSFullDepth = CreateDepthState(TRUE, TRUE, D3D11_COMPARISON_LESS_EQUAL);
    DSDepthNoWrite = CreateDepthState(TRUE, FALSE, D3D11_COMPARISON_LESS_EQUAL);
    DSNoDepth = CreateDepthState(FALSE, FALSE, D3D11_COMPARISON_ALWAYS);
    DSEqualDepthNoWrite = CreateDepthState(TRUE, FALSE, D3D11_COMPARISON_EQUAL);

    RSFullCull = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_FRONT, TRUE, FALSE);
    RSReverseCull = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK, TRUE, FALSE);
    RSNoCull = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_NONE, TRUE, FALSE);

    MainSampler = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_FLOAT32_MAX, 16);
    LMapSampler = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1);
    WarpSampler = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, 0, 1);
    DrawSampler = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1);
    CineSampler = CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER, 0, 1);
}

void StateManager::BindSamplers()
{
    // rebind in case state gets chucked
    ID3D11SamplerState* Samplers[] = {
        MainSampler.Get(),
        LMapSampler.Get(),
        WarpSampler.Get(),
        DrawSampler.Get(),
        CineSampler.Get()
    };

    RWGetDeviceContext()->PSSetSamplers(0, 5, Samplers);
}

void StateManager::SetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs)
{
    static ID3D11BlendState* oldbs = NULL;
    static ID3D11DepthStencilState* oldds = NULL;
    static ID3D11RasterizerState* oldrs = NULL;

    if (oldbs != EnumSelectBlendState(bs))
    {
        RWGetDeviceContext()->OMSetBlendState(EnumSelectBlendState(bs), NULL, 0xffffffff);
        oldbs = EnumSelectBlendState(bs);
    }

    if (oldds != EnumSelectDepthStencilState(ds))
    {
        RWGetDeviceContext()->OMSetDepthStencilState(EnumSelectDepthStencilState(ds), 0);
        oldds = EnumSelectDepthStencilState(ds);
    }

    if (oldrs != EnumSelectRasterizerState(rs))
    {
        RWGetDeviceContext()->RSSetState(EnumSelectRasterizerState(rs));
        oldrs = EnumSelectRasterizerState(rs);
    }
}

typedef struct streamdef_s {
    ID3D11Buffer* Buffer;
    UINT Stride;
    UINT Offset;
} streamdef_t;

void StateManager::BindVertexBuffer(UINT Slot, ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    static const int MAX_VERTEX_STREAMS = 16;
    static streamdef_t d3d_VertexStreams[MAX_VERTEX_STREAMS];

    if (Slot < 0) return;
    if (Slot >= MAX_VERTEX_STREAMS) return;

    if (d3d_VertexStreams[Slot].Buffer != Buffer ||
        d3d_VertexStreams[Slot].Stride != Stride ||
        d3d_VertexStreams[Slot].Offset != Offset)
    {
        RWGetDeviceContext()->IASetVertexBuffers(Slot, 1, &Buffer, &Stride, &Offset);

        d3d_VertexStreams[Slot].Buffer = Buffer;
        d3d_VertexStreams[Slot].Stride = Stride;
        d3d_VertexStreams[Slot].Offset = Offset;
    }
}


void StateManager::BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
    static ID3D11Buffer* OldBuffer = NULL;
    static DXGI_FORMAT OldFormat = DXGI_FORMAT_UNKNOWN;

    if (OldBuffer != Buffer || OldFormat != Format)
    {
        RWGetDeviceContext()->IASetIndexBuffer(Buffer, Format, 0);

        OldBuffer = Buffer;
        OldFormat = Format;
    }
}