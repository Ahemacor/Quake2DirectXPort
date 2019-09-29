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

class PipelineStateManager
{
public:
    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice);
    void Release();

    void RebuildState();

    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    void SetVertexShader(ShaderType shaderType);
    void SetPixelShader(ShaderType shaderType);
    void SetSampler(SamplerState samplerType);
    void SetInputLayout(InputLayout inputLayout);
    void SetBlendState(BlendState blendState);

    ID3D12RootSignature* GetRootSignature();
    ID3D12PipelineState* GetPSO();

    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHandle();
    ID3D12DescriptorHeap* GetSamplerDescriptorHeap();

    State currentState;
    bool isUpdateRequired = true;

private:
    void InitInputLayouts();
    void InitBlendStates();
    void InitDepthStates();
    void InitRasterizerStates();
    void InitSamplers();
    void CreateRootSignature();
    void CreatePipelineStateObject();
    std::wstring GetShaderFilepath(ShaderType shaderType);
    void LoadShader(ShaderType shaderType, const std::wstring& csoFilepath);
    ID3DBlob* GetShader(ShaderType shaderType);
    D3D12_BLEND_DESC CreateBlendState(BOOL blendon, D3D12_BLEND src, D3D12_BLEND dst, D3D12_BLEND_OP op);


    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap;
    std::map<ShaderType, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;
    D3D12_INPUT_LAYOUT_DESC inputLayouts[INPUT_LAYOUT_COUNT] = {};
    D3D12_BLEND_DESC blendStates[BlendState::BS_COUNT] = {};

};

