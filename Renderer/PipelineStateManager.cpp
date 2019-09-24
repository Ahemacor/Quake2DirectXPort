#include "PipelineStateManager.h"
#include "Utils.h"
#pragma comment(lib, "D3DCompiler.lib")
#include <d3dcompiler.h>

bool PipelineStateManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice)
{
    device = parentDevice;
    CreateSampler();
    CreateRootSignature();
    
    RebuildState();

    return !currentState.isUpdateRequired;
}

void PipelineStateManager::Release()
{
    device.Reset();
    rootSignature.Reset();
    pso.Reset();
    samplerDescriptorHeap.Reset();
    shaders.clear();
    currentState = {};
}

void PipelineStateManager::RebuildState()
{
    if (currentState.isUpdateRequired)
    {
        CreatePipelineStateObject();

        currentState.isUpdateRequired = false;
    }
}

void PipelineStateManager::SetPrimitiveTopology(PrimitiveTopologyType topology)
{
    currentState.topology = topology;
    currentState.isUpdateRequired = true;
}

void PipelineStateManager::SetVertexShader(ShaderType shaderType)
{
    currentState.VS = shaderType;
    currentState.isUpdateRequired = true;
}

void PipelineStateManager::SetPixelShader(ShaderType shaderType)
{
    currentState.PS = shaderType;
    currentState.isUpdateRequired = true;
}

void PipelineStateManager::SetSampler(SamplerState samplerType)
{
    currentState.sampler = samplerType;
    currentState.isUpdateRequired = true;
}

ID3D12RootSignature* PipelineStateManager::GetRootSignature()
{
    return rootSignature.Get();
}

ID3D12PipelineState* PipelineStateManager::GetPSO()
{
    return pso.Get();
}

ID3D12DescriptorHeap* PipelineStateManager::GetSamplerDescriptorHeap()
{
    return samplerDescriptorHeap.Get();
}

std::wstring PipelineStateManager::GetShaderFilepath(ShaderType shaderType)
{
    const std::wstring shaderDir = L"";

    std::wstring shaderFilename;
    switch (shaderType)
    {
    case ShaderType::VS_Test:
        shaderFilename = L"TestVertexShader.cso";
        break;

    case ShaderType::PS_Test:
        shaderFilename = L"TestPixelShader.cso";
        break;

    default:
        shaderFilename = L"";
        break;
    }
    ASSERT(!shaderFilename.empty());

    return shaderDir + shaderFilename;
}

void PipelineStateManager::CreateSampler()
{
    // Create sampler descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = 1;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ENSURE_RESULT(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerDescriptorHeap)));

    // Create sampler.
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    device->CreateSampler(&samplerDesc, samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void PipelineStateManager::CreateRootSignature()
{
    // Create root parameters and initialize.
    CD3DX12_ROOT_PARAMETER rootParameters[RootParameterIndex::RootParameterCount] = {};

    rootParameters[RootParameterIndex::ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[RootParameterIndex::TextureSRV].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_DESCRIPTOR_RANGE textureSampler(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    rootParameters[RootParameterIndex::TextureSampler].InitAsDescriptorTable(1, &textureSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    // Description.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};
    rsigDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // Serialize and create.
    Microsoft::WRL::ComPtr<ID3DBlob> rootBlob, errorBlob;
    ENSURE_RESULT(D3D12SerializeRootSignature(&rsigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob));
    ENSURE_RESULT(device->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void PipelineStateManager::CreatePipelineStateObject()
{
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(GetShader(currentState.VS));
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(GetShader(currentState.PS));
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.InputLayout.NumElements = std::extent<decltype(inputLayout)>::value;
    psoDesc.InputLayout.pInputElementDescs = inputLayout;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    // Simple alpha blending
    psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;
    psoDesc.SampleMask = 0xFFFFFFFF;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    ENSURE_RESULT(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void PipelineStateManager::LoadShader(ShaderType shaderType, const std::wstring& csoFilepath)
{
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    ENSURE_RESULT(D3DReadFileToBlob(csoFilepath.c_str(), shaderBlob.GetAddressOf()));
    shaders[shaderType] = shaderBlob;
}

ID3DBlob* PipelineStateManager::GetShader(ShaderType shaderType)
{
    if (shaders.find(shaderType) == shaders.cend())
    {
        LoadShader(shaderType, GetShaderFilepath(shaderType));
    }

    return shaders[shaderType].Get();
}