#include "PipelineStateManager.h"
#include "ResourceManager.h"
#include "Utils.h"
#pragma comment(lib, "D3DCompiler.lib")
#include <d3dcompiler.h>

#define DECL_VERTEX(name, fmt, slot) { name, 0, fmt, slot, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

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

void PipelineStateManager::SetRasterizerState(RasterizerState rasterizerState)
{
    if (currentState.RS != rasterizerState)
    {
        currentState.RS = rasterizerState;
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
    /*const UINT descrHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    CD3DX12_GPU_DESCRIPTOR_HANDLE samplerHandle(samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
                                                currentState.sampler,
                                                descrHandleSize);
    return samplerHandle;*/
    return samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

ID3D12DescriptorHeap* PipelineStateManager::GetSamplerDescriptorHeap()
{
    return samplerDescriptorHeap.Get();
}

void PipelineStateManager::InitInputLayouts()
{
    static const D3D12_INPUT_ELEMENT_DESC ilBeam[] =
    {
        DECL_VERTEX("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 7)
    };
    inputLayouts[INPUT_LAYOUT_BEAM].NumElements = std::size(ilBeam);
    inputLayouts[INPUT_LAYOUT_BEAM].pInputElementDescs = ilBeam;

    static const D3D12_INPUT_ELEMENT_DESC ilStandart[] =
    {
        DECL_VERTEX("POSITION", DXGI_FORMAT_R32G32_FLOAT, 0),
        DECL_VERTEX("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 0),
        DECL_VERTEX("COLOUR", DXGI_FORMAT_R8G8B8A8_UNORM, 0)
    };
    inputLayouts[INPUT_LAYOUT_STANDART].NumElements = std::size(ilStandart);
    inputLayouts[INPUT_LAYOUT_STANDART].pInputElementDescs = ilStandart;

    static const D3D12_INPUT_ELEMENT_DESC ilTexarray[] =
    {
        DECL_VERTEX("POSITION", DXGI_FORMAT_R32G32_FLOAT, 0),
        DECL_VERTEX("TEXCOORD", DXGI_FORMAT_R32G32B32_FLOAT, 0)
    };
    inputLayouts[INPUT_LAYOUT_TEXARRAY].NumElements = std::size(ilTexarray);
    inputLayouts[INPUT_LAYOUT_TEXARRAY].pInputElementDescs = ilTexarray;

    static const D3D12_INPUT_ELEMENT_DESC ilMesh[] =
    {
        DECL_VERTEX("PREVTRIVERTX", DXGI_FORMAT_R8G8B8A8_UINT, 1),
        DECL_VERTEX("CURRTRIVERTX", DXGI_FORMAT_R8G8B8A8_UINT, 2),
        DECL_VERTEX("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 3)
    };
    inputLayouts[INPUT_LAYOUT_MESH].NumElements = std::size(ilMesh);
    inputLayouts[INPUT_LAYOUT_MESH].pInputElementDescs = ilMesh;

    static const D3D12_INPUT_ELEMENT_DESC ilParticles[] =
    {
        DECL_VERTEX("ORIGIN", DXGI_FORMAT_R32G32B32_FLOAT, 6),
        DECL_VERTEX("VELOCITY", DXGI_FORMAT_R32G32B32_FLOAT, 6),
        DECL_VERTEX("ACCELERATION", DXGI_FORMAT_R32G32B32_FLOAT, 6),
        DECL_VERTEX("TIME", DXGI_FORMAT_R32_FLOAT, 6),
        DECL_VERTEX("COLOR", DXGI_FORMAT_R32_SINT, 6),
        DECL_VERTEX("ALPHA", DXGI_FORMAT_R32_FLOAT, 6)
    };
    inputLayouts[INPUT_LAYOUT_PARTICLES].NumElements = std::size(ilParticles);
    inputLayouts[INPUT_LAYOUT_PARTICLES].pInputElementDescs = ilParticles;

    static const D3D12_INPUT_ELEMENT_DESC ilSky[] =
    {
        DECL_VERTEX("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 4)
    };
    inputLayouts[INPUT_LAYOUT_SKY].NumElements = std::size(ilSky);
    inputLayouts[INPUT_LAYOUT_SKY].pInputElementDescs = ilSky;

    static const D3D12_INPUT_ELEMENT_DESC ilSprites[] =
    {
        DECL_VERTEX("XYOFFSET", DXGI_FORMAT_R32G32_FLOAT, 5)
    };
    inputLayouts[INPUT_LAYOUT_SPRITES].NumElements = std::size(ilSprites);
    inputLayouts[INPUT_LAYOUT_SPRITES].pInputElementDescs = ilSprites;

    static const D3D12_INPUT_ELEMENT_DESC ilSurface[] =
    {
        DECL_VERTEX("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 4),
        DECL_VERTEX("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 4),
        DECL_VERTEX("LIGHTMAP", DXGI_FORMAT_R16G16_UNORM, 4),
        DECL_VERTEX("STYLES", DXGI_FORMAT_R8G8B8A8_UINT, 4),
        DECL_VERTEX("MAPNUM", DXGI_FORMAT_R16_UINT, 4),
        DECL_VERTEX("SCROLL", DXGI_FORMAT_R16_UNORM, 4)
    };
    inputLayouts[INPUT_LAYOUT_SURFACES].NumElements = std::size(ilSurface);
    inputLayouts[INPUT_LAYOUT_SURFACES].pInputElementDescs = ilSurface;
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
    rasterizerStates[RasterizerState::RSFullCull] = CreateRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_FRONT, true, false);
    rasterizerStates[RasterizerState::RSReverseCull] = CreateRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, true, false);
    rasterizerStates[RasterizerState::RSNoCull] = CreateRasterizerState(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, true, false);
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
    // Create sampler state descriptions.
    samplerDesriptions[SamplerState::SAMPLER_MAIN] = CreateSamplerStateDescr(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FLOAT32_MAX, 16);
    samplerDesriptions[SamplerState::SAMPLER_L_MAP] = CreateSamplerStateDescr(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 1);
    samplerDesriptions[SamplerState::SAMPLER_WARP] = CreateSamplerStateDescr(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 1);
    samplerDesriptions[SamplerState::SAMPLER_DRAW] = CreateSamplerStateDescr(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 1);
    samplerDesriptions[SamplerState::SAMPLER_CINE] = CreateSamplerStateDescr(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 1);

    // Create sampler descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = SAMPLER_COUNT;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ENSURE_RESULT(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerDescriptorHeap)));

    const UINT descrHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (const auto& samplerStateDescr : samplerDesriptions)
    {
        device->CreateSampler(&samplerStateDescr, cpuHandle);
        cpuHandle.Offset(descrHandleSize);
    }

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
    psoDesc.RasterizerState = rasterizerStates[currentState.RS];

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
    D3D12_DEPTH_STENCIL_DESC desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

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

D3D12_RASTERIZER_DESC PipelineStateManager::CreateRasterizerState(D3D12_FILL_MODE fill, D3D12_CULL_MODE cull, bool clip, bool scissor)
{
    D3D12_RASTERIZER_DESC desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.FillMode = fill;
    desc.CullMode = cull;
    desc.FrontCounterClockwise = true;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0;
    desc.SlopeScaledDepthBias = 0;
    desc.DepthClipEnable = clip;
    desc.MultisampleEnable = false;
    desc.AntialiasedLineEnable = false;

    return desc;
}

D3D12_SAMPLER_DESC PipelineStateManager::CreateSamplerStateDescr(D3D12_FILTER Filter, D3D12_TEXTURE_ADDRESS_MODE AddressMode, float MaxLOD, UINT MaxAnisotropy)
{
    D3D12_SAMPLER_DESC desc = {};
    desc.AddressU = AddressMode;
    desc.AddressV = AddressMode;
    desc.AddressW = AddressMode;

    // border colour is always black because that's what our clamp-to-border code elsewhere in the engine expects
    desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0;

    // nope, not doing this
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    if (MaxAnisotropy > 1)
    {
        desc.Filter = D3D12_FILTER_ANISOTROPIC;
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

    return desc;
}