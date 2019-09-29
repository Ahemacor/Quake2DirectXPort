#include "PipelineStateManager.h"
#include "ResourceManager.h"
#include "Utils.h"
#pragma comment(lib, "D3DCompiler.lib")
#include <d3dcompiler.h>

bool PipelineStateManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> parentDevice)
{
    device = parentDevice;
    InitInputLayouts();
    InitBlendStates();
    InitDepthStates();
    InitRasterizerStates();
    InitSamplers();
    CreateRootSignature();

    return !isUpdateRequired;
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
    if (isUpdateRequired)
    {
        CreatePipelineStateObject();

        isUpdateRequired = false;
    }
}

void PipelineStateManager::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
{
    if (currentState.topology != topology)
    {
        currentState.topology = topology;
        isUpdateRequired = true;
    }
}

void PipelineStateManager::SetVertexShader(ShaderType shaderType)
{
    if (currentState.VS != shaderType)
    {
        currentState.VS = shaderType;
        isUpdateRequired = true;
    }
}

void PipelineStateManager::SetPixelShader(ShaderType shaderType)
{
    if (currentState.PS != shaderType)
    {
        currentState.PS = shaderType;
        isUpdateRequired = true;
    }
}

void PipelineStateManager::SetSampler(SamplerState samplerType)
{
    if (currentState.sampler != samplerType)
    {
        currentState.sampler = samplerType;
    }
}

void PipelineStateManager::SetInputLayout(InputLayout inputLayout)
{
    if (currentState.inputLayout != inputLayout)
    {
        currentState.inputLayout = inputLayout;
        isUpdateRequired = true;
    }
}

void PipelineStateManager::SetBlendState(BlendState blendState)
{
    if (currentState.BS != blendState)
    {
        currentState.BS = blendState;
        isUpdateRequired = true;
    }
}

void PipelineStateManager::SetDepthState(DepthStencilState depthState)
{
    if (currentState.DS != depthState)
    {
        currentState.DS = depthState;
        isUpdateRequired = true;
    }
}

ID3D12RootSignature* PipelineStateManager::GetRootSignature()
{
    return rootSignature.Get();
}

ID3D12PipelineState* PipelineStateManager::GetPSO()
{
    return pso.Get();
}

D3D12_GPU_DESCRIPTOR_HANDLE PipelineStateManager::GetSamplerHandle()
{
    const UINT descrHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
                                                currentState.sampler,
                                                descrHandleSize);
    return samplerHandle;
}

ID3D12DescriptorHeap* PipelineStateManager::GetSamplerDescriptorHeap()
{
    return samplerDescriptorHeap.Get();
}

void PipelineStateManager::InitInputLayouts()
{
    D3D12_INPUT_LAYOUT_DESC inputLayoutDescr;

    // TestInputLayout
    static const D3D12_INPUT_ELEMENT_DESC testInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    inputLayoutDescr.NumElements = ARRAYSIZE(testInputLayout);
    inputLayoutDescr.pInputElementDescs = testInputLayout;
    inputLayouts[INPUT_LAYOUT_TEST] = inputLayoutDescr;

    // ...
}

void PipelineStateManager::InitBlendStates()
{
    blendStates[BlendState::BSNone] = CreateBlendState(false, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD);
    blendStates[BlendState::BSAlphaBlend] = CreateBlendState(true, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD);
    blendStates[BlendState::BSAlphaReverse] = CreateBlendState(true, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_OP_ADD);
    blendStates[BlendState::BSAlphaPreMult] = CreateBlendState(true, D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD);
    blendStates[BlendState::BSAdditive] = CreateBlendState(true, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD);
    blendStates[BlendState::BSRevSubtract] = CreateBlendState(true, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_REV_SUBTRACT);
}

void PipelineStateManager::InitDepthStates()
{
    depthStancilStates[DepthStencilState::DSFullDepth] = CreateDepthState(true, true, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    depthStancilStates[DepthStencilState::DSDepthNoWrite] = CreateDepthState(true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    depthStancilStates[DepthStencilState::DSNoDepth] = CreateDepthState(false, false, D3D12_COMPARISON_FUNC_ALWAYS);
    depthStancilStates[DepthStencilState::DSEqualDepthNoWrite] = CreateDepthState(true, false, D3D12_COMPARISON_FUNC_EQUAL);
}

void PipelineStateManager::InitRasterizerStates()
{

}

std::wstring PipelineStateManager::GetShaderFilepath(ShaderType shaderType)
{
    const std::wstring shaderDir = L"";

    std::wstring shaderFilename;
    switch (shaderType)
    {
    case ShaderType::SHADER_TEST_VS:
        shaderFilename = L"TestVertexShader.cso";
        break;

    case ShaderType::SHADER_TEST_PS:
        shaderFilename = L"TestPixelShader.cso";
        break;

    default:
        shaderFilename = L"";
        break;
    }
    ASSERT(!shaderFilename.empty());

    return shaderDir + shaderFilename;
}

void PipelineStateManager::InitSamplers()
{
    // Create sampler descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = SAMPLER_COUNT;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ENSURE_RESULT(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerDescriptorHeap)));

    const UINT descrHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // DefaultSampler
    cpuHandle.Offset(SAMPLER_DEFAULT, descrHandleSize);
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
    device->CreateSampler(&samplerDesc, cpuHandle);
}

void PipelineStateManager::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ResourceManager::DESCR_HEAP_MAX, 0);
    CD3DX12_DESCRIPTOR_RANGE samplerRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_COUNT, 0);

    // Create root parameters and initialize.
    CD3DX12_ROOT_PARAMETER rootParameters[ParameterIdx::ROOT_PARAMS_COUNT] = {};
    rootParameters[ParameterIdx::SRV_TABLE_IDX].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ParameterIdx::SAMPLERS_TABLE_IDX].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ParameterIdx::CB0_IDX].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB1_IDX].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB2_IDX].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB3_IDX].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB4_IDX].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB5_IDX].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB6_IDX].InitAsConstantBufferView(6, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB7_IDX].InitAsConstantBufferView(7, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB8_IDX].InitAsConstantBufferView(8, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[ParameterIdx::CB9_IDX_MAX].InitAsConstantBufferView(9, 0, D3D12_SHADER_VISIBILITY_ALL);

    // Description.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

    // Serialize and create.
    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};
    rsigDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
    Microsoft::WRL::ComPtr<ID3DBlob> rootBlob, errorBlob;
    ENSURE_RESULT(D3D12SerializeRootSignature(&rsigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob));
    ENSURE_RESULT(device->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void PipelineStateManager::CreatePipelineStateObject()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(GetShader(currentState.VS));
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(GetShader(currentState.PS));
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    psoDesc.InputLayout = inputLayouts[currentState.inputLayout];

    psoDesc.BlendState = blendStates[currentState.BS];

    psoDesc.DepthStencilState = depthStancilStates[currentState.DS];

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = 0xFFFFFFFF;

    psoDesc.PrimitiveTopologyType = currentState.topology;
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

D3D12_BLEND_DESC PipelineStateManager::CreateBlendState(bool blendon, D3D12_BLEND src, D3D12_BLEND dst, D3D12_BLEND_OP op)
{
    D3D12_BLEND_DESC desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.AlphaToCoverageEnable = false;
    desc.IndependentBlendEnable = false;
    desc.RenderTarget[0].BlendEnable = blendon;
    desc.RenderTarget[0].SrcBlend = src;
    desc.RenderTarget[0].DestBlend = dst;
    desc.RenderTarget[0].BlendOp = op;
    desc.RenderTarget[0].SrcBlendAlpha = src;
    desc.RenderTarget[0].DestBlendAlpha = dst;
    desc.RenderTarget[0].BlendOpAlpha = op;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return desc;
}

D3D12_DEPTH_STENCIL_DESC PipelineStateManager::CreateDepthState(bool test, bool mask, D3D12_COMPARISON_FUNC func)
{
    D3D12_DEPTH_STENCIL_DESC desc = {};

    if (test)
    {
        desc.DepthWriteMask = mask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    }
    else
    {
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    desc.DepthEnable = test;
    desc.DepthFunc = func;
    desc.StencilEnable = false;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

    return desc;
}