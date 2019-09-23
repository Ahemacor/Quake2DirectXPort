#include "Renderer.h"
#include "Utils.h"
#include "ImageIO/ImageIO.h"
#include <cassert>



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

Renderer::Renderer() : vertexBufferView(), indexBufferView(), imageData() {}

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    pRenderEnv->ResetCommandList();
    stateManager.SetVertexShader(PipelineStateManager::VS_Test);
    stateManager.SetPixelShader(PipelineStateManager::PS_Test);
    stateManager.Initialize(pRenderEnv->GetDevice());
    CreateTestMesh();
    CreateTexture();
    
    pRenderEnv->ExecuteCommandList();
            
    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();

    vertexBuffer.Reset();
    vertexBufferView = {};

    indexBuffer.Reset();
    indexBufferView = {};

    image.Reset();
    imageData = {};
    descriptorHeap.Reset();

    isInitialized = false;
}

void Renderer::RenderImpl()
{
    assert(isInitialized == true);

    auto commandList = pRenderEnv->GetGraphicsCommandList();

    stateManager.RebuildState();
    commandList->SetPipelineState(stateManager.GetPSO());
    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());


    // Set the descriptor heap containing the texture srv
    ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get(), stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSRV, descriptorHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSampler, stateManager.GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    pRenderEnv->Synchronize();
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

void Renderer::CreateTexture()
{
    // Create Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = 1;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

    // Load image binary Data
    int width = 0, height = 0;
    const char* imageFilePath = "ruby.jpg";
    imageData = LoadImageFromFile(imageFilePath, 1, &width, &height);

    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1),
                                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                                    nullptr,
                                                    IID_PPV_ARGS(&image)));

    const auto uploadBufferSize = GetRequiredIntermediateSize(image.Get(), 0, 1);
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    nullptr,
                                                    IID_PPV_ARGS(&uploadImage)));

    D3D12_SUBRESOURCE_DATA srcData;
    srcData.pData = imageData.data();
    srcData.RowPitch = width * 4;
    srcData.SlicePitch = width * height * 4;

    auto uploadCommandList = pRenderEnv->GetGraphicsCommandList();
    UpdateSubresources(uploadCommandList.Get(), image.Get(), uploadImage.Get(), 0, 0, 1, &srcData);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(image.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    uploadCommandList->ResourceBarrier(1, &transition);

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    pRenderEnv->GetDevice()->CreateShaderResourceView(image.Get(),
                                                      &shaderResourceViewDesc,
                                                      descriptorHeap->GetCPUDescriptorHandleForHeapStart());
}