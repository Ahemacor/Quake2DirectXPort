#include "Renderer.h"
#include "Utils.h"
#include <cassert>

static inline void CloseWithError(const char* errorLine)
{
    std::string errMsg = "Not implemented error: ";
    errMsg += errorLine;
    errMsg += "\n";
    MessageBoxA(NULL, errMsg.c_str(), "Error", MB_ICONERROR);
    OutputDebugStringA(errMsg.c_str());
    exit(-1);
}

#define NOT_IMPL_FAIL() (CloseWithError(__FUNCTION__))

Renderer::Renderer() {}

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    stateManager.Initialize(pRenderEnv->GetDevice());
    resourceManager.Initialize(pRenderEnv);

    vertexBufferView = {};
    indexBufferView = {};

    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();
    resourceManager.Release();
    cbArguments.clear();
    vertexBufferView = {};
    indexBufferView = {};
    isInitialized = false;
}

void Renderer::Draw(UINT numOfVertices, UINT firstVertexToDraw)
{
    NOT_IMPL_FAIL();
}

void Renderer::DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation)
{
    stateManager.RebuildState();

    auto commandList = pRenderEnv->GetGraphicsCommandList();

    commandList->SetPipelineState(stateManager.GetPSO());
    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(),
                                      stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::ParameterIdx::SAMPLERS_TABLE_IDX, stateManager.GetSamplerHandle());

    for (const auto& pair : cbArguments)
    {
        commandList->SetGraphicsRootConstantBufferView(PipelineStateManager::ParameterIdx::CB0_IDX + pair.first, pair.second);
    }
    
    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::ParameterIdx::SRV_TABLE_IDX, resourceManager.GetSrvHandle());

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->DrawIndexedInstanced(indexCount, 1, firstIndex, baseVertexLocation, 0);

    pRenderEnv->Synchronize();
}

// CREATE METHODS:

ResourceManager::Resource::Id Renderer::CreateConstantBuffer(const std::size_t bufferSize, const void* pSrcData)
{
    ResourceManager::Resource resource;
    resource.type = ResourceManager::Resource::Type::CB;
    const CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    if (pSrcData != nullptr)
    {
        resourceManager.UpdateBufferData(resource.d12resource.Get(), pSrcData, bufferSize);
    }
    resource.variant.cbHandle = resource.d12resource.Get()->GetGPUVirtualAddress();
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    return resourceManager.AddResource(resource);
}

ResourceManager::Resource::Id Renderer::CreateTextureResource(const std::size_t width, const std::size_t height, const void* pImageData)
{
    ResourceManager::Resource resource;
    resource.type = ResourceManager::Resource::Type::SRV;
    CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1);
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    if (pImageData != nullptr)
    {
        resourceManager.UpdateSRVBuffer(resource.d12resource.Get(), pImageData, width, height);
    }
    resource.variant.imageSize.width = width;
    resource.variant.imageSize.height = height;
    //resource.variant.srvHandle = resourceManager.CreateShaderResourceView(resource.d12resource.Get(), width, height);
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    return resourceManager.AddResource(resource);
}

ResourceManager::Resource::Id Renderer::CreateVertexBuffer(const std::size_t numOfVertices, const std::size_t vertexSize, const void* pVertexData)
{
    ResourceManager::Resource resource;
    resource.type = ResourceManager::Resource::Type::VB;
    const std::size_t dataSize = vertexSize * numOfVertices;
    CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    if (pVertexData != nullptr)
    {
        resourceManager.UpdateBufferData(resource.d12resource.Get(), pVertexData, dataSize);
    }
    resource.variant.vbView = resourceManager.CreateVertexBufferView(resource.d12resource.Get(), dataSize, vertexSize);
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    return resourceManager.AddResource(resource);
}

ResourceManager::Resource::Id Renderer::CreateIndexBuffer(const std::size_t numOfIndices, const void* pIndexData, const std::size_t indexSize)
{
    ResourceManager::Resource resource;
    resource.type = ResourceManager::Resource::Type::IB;
    const std::size_t dataSize = numOfIndices * indexSize;
    CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    if (pIndexData != nullptr)
    {
        resourceManager.UpdateBufferData(resource.d12resource.Get(), pIndexData, dataSize);
    }
    resource.variant.ibView = resourceManager.CreateIndexBufferView(resource.d12resource.Get(), dataSize);
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    return resourceManager.AddResource(resource);
}

// UPDATE METHODS:

void Renderer::UpdateConstantBuffer(ResourceManager::Resource::Id resourceId, const void* pSrcData, const std::size_t bufferSize)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pSrcData, bufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    resource.variant.cbHandle = resource.d12resource.Get()->GetGPUVirtualAddress();
}

void Renderer::UpdateTextureResource(ResourceManager::Resource::Id resourceId, const void* pImageData, const std::size_t width, const std::size_t height)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateSRVBuffer(resource.d12resource.Get(), pImageData, width, height, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //resource.variant.srvHandle = resourceManager.CreateShaderResourceView(resource.d12resource.Get(), width, height);
    resource.variant.imageSize.width = width;
    resource.variant.imageSize.height = height;
}

void Renderer::UpdateVertexBuffer(ResourceManager::Resource::Id resourceId, const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    const std::size_t dataSize = vertexSize * numOfVertices;
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pVertexData, dataSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    resource.variant.vbView = resourceManager.CreateVertexBufferView(resource.d12resource.Get(), dataSize, vertexSize);
}

void Renderer::UpdateIndexBuffer(ResourceManager::Resource::Id resourceId, const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize)
{
    const std::size_t dataSize = numOfIndices * indexSize;
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pIndexData, dataSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    resource.variant.ibView = resourceManager.CreateIndexBufferView(resource.d12resource.Get(), dataSize);
}

// BIND METHODS:

void Renderer::BindConstantBuffer(ResourceManager::Resource::Id resourceId, std::size_t slot)
{
    const std::size_t CB_SLOT_MAX = PipelineStateManager::ParameterIdx::CB9_IDX_MAX - PipelineStateManager::ParameterIdx::CB0_IDX;
    ASSERT(slot <= CB_SLOT_MAX);
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::CB);
    cbArguments[slot] = resource.variant.cbHandle;
}

void Renderer::BindTextureResource(ResourceManager::Resource::Id resourceId, std::size_t slot)
{
    ASSERT(slot < ResourceManager::DESCR_HEAP_MAX);
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::SRV);
    //srvArguments[slot] = resource.variant.srvHandle;
    resourceManager.CreateShaderResourceView(resource.d12resource.Get(), resource.variant.imageSize.width, resource.variant.imageSize.height, slot);
}

void Renderer::BindVertexBuffer(ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::VB);
    vertexBufferView = resource.variant.vbView;
}

void Renderer::BindIndexBuffer(ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::IB);
    indexBufferView = resource.variant.ibView;
}

void Renderer::SetPrimitiveTopologyTriangleList()
{
    stateManager.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
}