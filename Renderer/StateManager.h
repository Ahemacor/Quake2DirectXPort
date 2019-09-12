#pragma once

#ifdef __cplusplus
#undef CINTERFACE

#include "r_local.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <windows.h>

class StateManager
{
public:
    void SetDefaultState();
    void BindSamplers();

    void SetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs);

    void BindVertexBuffer(UINT Slot, ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format);

private:
    ID3D11BlendState* EnumSelectBlendState(BlendState bs);
    ID3D11DepthStencilState* EnumSelectDepthStencilState(DepthStencilState ds);
    ID3D11RasterizerState* EnumSelectRasterizerState(RasterizerState rs);

    Microsoft::WRL::ComPtr<ID3D11SamplerState> CreateSamplerState(D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressMode, float MaxLOD, UINT MaxAnisotropy);
    Microsoft::WRL::ComPtr<ID3D11BlendState> CreateBlendState(BOOL blendon, D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> CreateDepthState(BOOL test, BOOL mask, D3D11_COMPARISON_FUNC func);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> CreateRasterizerState(D3D11_FILL_MODE fill, D3D11_CULL_MODE cull, BOOL clip, BOOL scissor);

    Microsoft::WRL::ComPtr<ID3D11BlendState> BSNone;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BSAlphaBlend;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BSAlphaReverse;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BSAlphaPreMult;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BSAdditive;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BSRevSubtract;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DSFullDepth;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DSDepthNoWrite;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DSNoDepth;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DSEqualDepthNoWrite;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RSFullCull;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RSReverseCull;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RSNoCull;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> MainSampler;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> LMapSampler;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> WarpSampler;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> DrawSampler;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> CineSampler;
};

#endif

