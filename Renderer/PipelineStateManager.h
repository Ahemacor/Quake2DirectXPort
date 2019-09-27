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
    enum ParameterIdx
    {
        SRV_TABLE_IDX = 0,

        SAMPLERS_TABLE_IDX,

        CB0_IDX,
        CB1_IDX,
        CB2_IDX,
        CB3_IDX,
        CB4_IDX,
        CB5_IDX,
        CB6_IDX,
        CB7_IDX,
        CB8_IDX,
        CB9_IDX,

        ROOT_PARAMS_COUNT
    };

    enum ShaderType
    {
        Undefined = 0,
        VS_Test,
        PS_Test,

        SHADER_TYPE_COUNT
    };

    enum SamplerState
    {
        DefaultSampler = 0,

        SAMPLER_COUNT
    };

    enum InputLayout
    {
        TestInputLayout = 0,

        INPUT_LAYOUT_COUNT
    };

    struct State
    {
        InputLayout inputLayout = InputLayout::TestInputLayout;

        ShaderType VS = ShaderType::Undefined;
        ShaderType PS = ShaderType::Undefined;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        SamplerState sampler = SamplerState::DefaultSampler;

        bool isUpdateRequired = true;
    };

    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice);
    void Release();

    void RebuildState();

    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    void SetVertexShader(ShaderType shaderType);
    void SetPixelShader(ShaderType shaderType);
    void SetSampler(SamplerState samplerType);
    void SetInputLayout(InputLayout inputLayout);

    ID3D12RootSignature* GetRootSignature();
    ID3D12PipelineState* GetPSO();

    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHandle();
    ID3D12DescriptorHeap* GetSamplerDescriptorHeap();

    State currentState;

private:
    void InitInputLayouts();
    void InitSamplers();
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
    D3D12_INPUT_LAYOUT_DESC inputLayouts[INPUT_LAYOUT_COUNT] = {};

};

