#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"
#include "r_local.h"
#include <map>
#include <string>

bool operator==(const State& lhs, const State& rhs);

class PipelineStateManager
{
public:
    struct RenderState
    {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
        State state;
    };

    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice);
    void Release();

    ID3D12RootSignature* GetRootSignature();

    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHandle();
    ID3D12DescriptorHeap* GetSamplerDescriptorHeap();

    UINT CreatePipelineStateObject(const State& state, int stateId = -1);
    ID3D12PipelineState* GetPSO(UINT stateId);
    State GetStateDescr(UINT stateId);

    bool isUpdateRequired = true;

private:
    void InitInputLayouts();
    void InitBlendStates();
    void InitDepthStates();
    void InitRasterizerStates();
    void InitSamplers();
    void CreateRootSignature();
    std::wstring GetShaderFilepath(ShaderType shaderType);
    void LoadShader(ShaderType shaderType, const std::wstring& csoFilepath);
    D3D12_SHADER_BYTECODE GetShader(ShaderType shaderType);
    D3D12_BLEND_DESC CreateBlendState(bool blendon, D3D12_BLEND src, D3D12_BLEND dst, D3D12_BLEND_OP op);
    D3D12_DEPTH_STENCIL_DESC CreateDepthState(bool test, bool mask, D3D12_COMPARISON_FUNC func);
    D3D12_RASTERIZER_DESC CreateRasterizerState(D3D12_FILL_MODE fill, D3D12_CULL_MODE cull, bool clip, bool scissor);
    D3D12_SAMPLER_DESC CreateSamplerStateDescr(D3D12_FILTER Filter, D3D12_TEXTURE_ADDRESS_MODE AddressMode, float MaxLOD, UINT MaxAnisotropy);

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    std::vector<RenderState> PSOs;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap;
    std::map<ShaderType, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;
    D3D12_INPUT_LAYOUT_DESC inputLayouts[INPUT_LAYOUT_COUNT] = {};
    D3D12_BLEND_DESC blendStates[BlendState::BS_COUNT] = {};
    D3D12_DEPTH_STENCIL_DESC depthStancilStates[DepthStencilState::DS_COUNT] = {};
    D3D12_RASTERIZER_DESC rasterizerStates[RasterizerState::RS_COUNT] = {};
    D3D12_SAMPLER_DESC samplerDesriptions[SamplerState::SAMPLER_COUNT] = {};

};

