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

    vertexBufferToBind = {};
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
    vertexBufferToBind = {};
    indexBufferView = {};
    isInitialized = false;
}

void Renderer::Draw(UINT numOfVertices, UINT firstVertexToDraw)
{
    auto commandList = pRenderEnv->GetRenderCommandList();

    commandList->SetPipelineState(stateManager.GetPSO(psoId));
    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(),
                                      stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SAMPLERS_TABLE_IDX, stateManager.GetSamplerHandle());

    for (const auto& pair : cbArguments)
    {
        commandList->SetGraphicsRootConstantBufferView(ParameterIdx::CB0_IDX + pair.first, pair.second);
    }

    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SRV_TABLE_IDX, resourceManager.GetSrvHandle());

    commandList->IASetVertexBuffers(vertexBufferToBind.slot, 1, &vertexBufferToBind.view);

    commandList->DrawInstanced(numOfVertices, 1, firstVertexToDraw, 0);
}

/*
Microsoft::WRL::ComPtr<ID3D12Resource> testIndexBuffer;
Microsoft::WRL::ComPtr<ID3D12Resource> testUploadBuffer;
D3D12_INDEX_BUFFER_VIEW testIndexBufferVies = {};
static const int indices[6] = { 0, 1, 2, 3, 4, 5 };
*/

void Renderer::DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation)
{
    auto commandList = pRenderEnv->GetRenderCommandList();

    commandList->SetPipelineState(stateManager.GetPSO(psoId));
    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(),
                                      stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SAMPLERS_TABLE_IDX, stateManager.GetSamplerHandle());

    for (const auto& pair : cbArguments)
    {
        commandList->SetGraphicsRootConstantBufferView(ParameterIdx::CB0_IDX + pair.first, pair.second);
    }
    
    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SRV_TABLE_IDX, resourceManager.GetSrvHandle());

/*
    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    static const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));
    pRenderEnv->GetDevice()->CreateCommittedResource(&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&testIndexBuffer));

    testIndexBufferVies.BufferLocation = testIndexBuffer->GetGPUVirtualAddress();
    testIndexBufferVies.SizeInBytes = sizeof(indices);
    testIndexBufferVies.Format = DXGI_FORMAT_R32_UINT;

    pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&testUploadBuffer));

    void* p;
    testUploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, indices, sizeof(indices));
    testUploadBuffer->Unmap(0, nullptr);

    commandList->CopyBufferRegion(testIndexBuffer.Get(), 0, testUploadBuffer.Get(), 0, sizeof(indices));

    const CD3DX12_RESOURCE_BARRIER barriers[1] = { CD3DX12_RESOURCE_BARRIER::Transition(testIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER) };
    commandList->ResourceBarrier(1, barriers);

    commandList->IASetIndexBuffer(&testIndexBufferVies);

*/
    // commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(vertexBufferToBind.slot, 1, &vertexBufferToBind.view);
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->DrawIndexedInstanced(indexCount, 1, firstIndex, baseVertexLocation, 0);
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

ResourceManager::Resource::Id Renderer::CreateTextureResource(const CD3DX12_RESOURCE_DESC& descr, D3D12_SUBRESOURCE_DATA* pSrcData)
{
    ResourceManager::Resource resource;
    resource.type = ResourceManager::Resource::Type::SRV;
    resource.variant.texDescr = descr;
    //CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1);
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    if (pSrcData != nullptr)
    {
        resourceManager.UpdateSRVBuffer(resource.d12resource.Get(), pSrcData);
    }
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

void Renderer::UpdateTextureResource(ResourceManager::Resource::Id resourceId, D3D12_SUBRESOURCE_DATA* pSrcData)
{
    ASSERT(resourceId != 0);
    ASSERT(pSrcData != nullptr);
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateSRVBuffer(resource.d12resource.Get(), pSrcData, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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
    const std::size_t CB_SLOT_MAX = ParameterIdx::CB9_IDX_MAX - ParameterIdx::CB0_IDX;
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
    resourceManager.CreateShaderResourceView(resourceId, slot);
}

void Renderer::BindVertexBuffer(UINT Slot, ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::VB);
    vertexBufferToBind.slot = Slot;
    vertexBufferToBind.view = resource.variant.vbView;
}

void Renderer::BindIndexBuffer(ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::IB);
    indexBufferView = resource.variant.ibView;
}

UINT Renderer::CreatePSO(const State* psoState)
{
    return stateManager.CreatePipelineStateObject(*psoState);
}

void Renderer::SetPSO(UINT PSOid)
{
    psoId = PSOid;
}