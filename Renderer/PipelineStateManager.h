#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"
#include <map>
#include <string>

class PipelineStateManager
{
public:
    enum RootParameterIndex
    {
        ConstantBuffer,
        TextureSRV,
        TextureSampler,

        RootParameterCount
    };

    enum ShaderType
    {
        Undefined = 0,
        VS_Test,
        PS_Test,

        ShaderTypeCount
    };

    enum PrimitiveTopologyType
    {
        TriangleList
    };

    enum SamplerState
    {
        DefaultSampler
    };

    struct State
    {
        ShaderType VS = ShaderType::Undefined;
        ShaderType PS = ShaderType::Undefined;
        PrimitiveTopologyType topology = PrimitiveTopologyType::TriangleList;
        SamplerState sampler = SamplerState::DefaultSampler;

        bool isUpdateRequired = true;
    };

    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice);
    void Release();

    void RebuildState();

    void SetPrimitiveTopology(PrimitiveTopologyType topology);
    void SetVertexShader(ShaderType shaderType);
    void SetPixelShader(ShaderType shaderType);
    void SetSampler(SamplerState samplerType);

    ID3D12RootSignature* GetRootSignature();
    ID3D12PipelineState* GetPSO();
    ID3D12DescriptorHeap* GetSamplerDescriptorHeap();

private:
    void CreateSampler();
    void CreateRootSignature();
    void CreatePipelineStateObject();
    std::wstring GetShaderFilepath(ShaderType shaderType);
    void LoadShader(ShaderType shaderType, const std::wstring& csoFilepath);
    ID3DBlob* GetShader(ShaderType shaderType);

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap;
    std::map<ShaderType, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;

    State currentState;
};
