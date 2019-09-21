#include "Renderer.h"
#include "Utils.h"
#include <cassert>
#pragma comment(lib, "D3DCompiler.lib")
#include <d3dcompiler.h>
#include <string>


struct Vertex
{
    float position[3];
    float uv[2];
};

// Declare upload buffer data as 'static' so it persists after returning from this function.
// Otherwise, we would need to explicitly wait for the GPU to copy data from the upload buffer
// to vertex/index default buffers due to how the GPU processes commands asynchronously. 
static const Vertex vertices[4] = {
    // Upper Left
    { { -1.0f, 1.0f, 0 },{ 0, 0 } },
    // Upper Right
    { { 1.0f, 1.0f, 0 },{ 1, 0 } },
    // Bottom right
    { { 1.0f, -1.0f, 0 },{ 1, 1 } },
    // Bottom left
    { { -1.0f, -1.0f, 0 },{ 0, 1 } }
};

static const int indices[6] = {
    0, 1, 2, 2, 3, 0
};

static Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

void Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    pRenderEnv->ResetCommandList();

    CreateRootSignature();
    CreatePipelineStateObject();
    CreateTestMesh();
    
    pRenderEnv->ExecuteCommandList();
}

void Renderer::Release()
{

}

void Renderer::RenderImpl()
{
    auto commandList = pRenderEnv->GetGraphicsCommandList();
    commandList->SetPipelineState(pso.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    pRenderEnv->Synchronize();
}

void Renderer::CreateRootSignature()
{
    CD3DX12_ROOT_PARAMETER parameters[1];
    parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

    // Create the root signature
    descRootSignature.Init(1, parameters,
        0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> rootBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    ENSURE_RESULT(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob));
    ENSURE_RESULT( pRenderEnv->GetDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Renderer::CreatePipelineStateObject()
{
    std::wstring shaderDir = LR"(C:\CPP\RenderingTask\Debug\)";
    std::wstring vertexShaderFilename = L"TestVertexShader.cso";
    std::wstring pixelShaderFilename = L"TestPixelShader.cso";

    std::wstring VSPath = shaderDir + vertexShaderFilename;
    std::wstring PSPath = shaderDir + pixelShaderFilename;

    ENSURE_RESULT(D3DReadFileToBlob(VSPath.c_str(), vertexShaderBlob.GetAddressOf()));
    ENSURE_RESULT(D3DReadFileToBlob(PSPath.c_str(), pixelShaderBlob.GetAddressOf()));

    static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
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

    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void Renderer::CreateTestMesh()
{
    static const int uploadBufferSize = sizeof(vertices) + sizeof(indices);

    // Create upload buffer on CPU
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                     nullptr,
                                                     IID_PPV_ARGS(&uploadBuffer)));

    // Create vertex & index buffer on the GPU
    // HEAP_TYPE_DEFAULT is on GPU, we also initialize with COPY_DEST state
    // so we don't have to transition into this before copying into them
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                     nullptr,
                                                     IID_PPV_ARGS(&vertexBuffer)));

    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                     nullptr,
                                                     IID_PPV_ARGS(&indexBuffer)));

    // Create buffer views
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(vertices);
    vertexBufferView.StrideInBytes = sizeof(Vertex);

    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = sizeof(indices);
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // Copy data on CPU into the upload buffer
    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, vertices, sizeof(vertices));
    ::memcpy(static_cast<unsigned char*>(p) + sizeof(vertices),
        indices, sizeof(indices));
    uploadBuffer->Unmap(0, nullptr);

    auto commandList = pRenderEnv->GetGraphicsCommandList();

    // Copy data from upload buffer on CPU into the index/vertex buffer on 
    // the GPU
    commandList->CopyBufferRegion(vertexBuffer.Get(), 0,
        uploadBuffer.Get(), 0, sizeof(vertices));
    commandList->CopyBufferRegion(indexBuffer.Get(), 0,
        uploadBuffer.Get(), sizeof(vertices), sizeof(indices));

    // Barriers, batch them together
    const CD3DX12_RESOURCE_BARRIER barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
    };

    commandList->ResourceBarrier(2, barriers);
}