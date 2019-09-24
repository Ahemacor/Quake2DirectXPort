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

Renderer::Renderer() {}

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    stateManager.SetVertexShader(PipelineStateManager::VS_Test);
    stateManager.SetPixelShader(PipelineStateManager::PS_Test);
    stateManager.Initialize(pRenderEnv->GetDevice());
    resourceManager.Initialize(pRenderEnv);

    pRenderEnv->ResetCommandList();

    resourceManager.CreateVertexBuffer(vertices, sizeof(Vertex), ARRAYSIZE(vertices));
    resourceManager.CreateIndexBuffer(indices, ARRAYSIZE(indices));

    //CreateTexture();
    int width = 0, height = 0;
    const char* imageFilePath = "ruby.jpg";
    std::vector<std::uint8_t> imageData = LoadImageFromFile(imageFilePath, 1, &width, &height);
    resourceManager.CreateSRVBuffer(imageData.data(), width, height);
    
    pRenderEnv->ExecuteCommandList();
            
    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();
    resourceManager.Release();

    /*imageResource.Reset();
    imageData = {};
    descriptorHeap.Reset();*/

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
    //ID3D12DescriptorHeap* heaps[] = { descriptorHeap.Get(), stateManager.GetSamplerDescriptorHeap() };
    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(), stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSRV, resourceManager.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSampler, stateManager.GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &(resourceManager.GetVertexBufferView(0)));
    commandList->IASetIndexBuffer(&resourceManager.GetIndexBufferView(0));
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    pRenderEnv->Synchronize();
}

/*void Renderer::CreateTexture()
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
                                                    IID_PPV_ARGS(&imageResource)));

    const auto uploadBufferSize = GetRequiredIntermediateSize(imageResource.Get(), 0, 1);
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
    UpdateSubresources(uploadCommandList.Get(), imageResource.Get(), uploadImage.Get(), 0, 0, 1, &srcData);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(imageResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    uploadCommandList->ResourceBarrier(1, &transition);

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    pRenderEnv->GetDevice()->CreateShaderResourceView(imageResource.Get(),
                                                      &shaderResourceViewDesc,
                                                      descriptorHeap->GetCPUDescriptorHandleForHeapStart());
}*/