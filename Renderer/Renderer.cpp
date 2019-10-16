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

    indexBufferView = {};

    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    if (isInitialized)
    {
        pRenderEnv = nullptr;
        resourceManager.Release();
        stateManager.Release();
        cbArguments.clear();
        srvArguments.clear();
        mappedSrv.clear();
        vertexBuffers.clear();
        indexBufferView = {};
        isInitialized = false;
    }
}

void Renderer::CommonDraw(ID3D12GraphicsCommandList* commandList)
{
    // Init SRVs
    for (const auto& srvPair : srvArguments)
    {
        const auto& slot = srvPair.first;
        const auto& srvResId = srvPair.second;
        ASSERT(slot < ResourceManager::DESCR_HEAP_MAX);

        auto findIt = mappedSrv.find(slot);
        if (findIt == mappedSrv.cend() || findIt->second != srvResId)
        {
            mappedSrv[slot] = srvResId;
            resourceManager.CreateShaderResourceView(srvResId, slot);
        }
    }

    commandList->SetPipelineState(stateManager.GetPSO(psoId));

    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(),
                                      stateManager.GetSamplerDescriptorHeap() };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SRV_TABLE_IDX, resourceManager.GetSrvHandle());

    commandList->SetGraphicsRootDescriptorTable(ParameterIdx::SAMPLERS_TABLE_IDX, stateManager.GetSamplerHandle());

    for (const auto& pair : cbArguments)
    {
        commandList->SetGraphicsRootConstantBufferView(ParameterIdx::CB0_IDX + pair.first, pair.second);
    }

    if (stateManager.GetStateDescr(psoId).topology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
    else if (stateManager.GetStateDescr(psoId).topology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT)
    {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    }

    for (const auto vbPair : vertexBuffers)
    {
        const auto& vbSlot = vbPair.first;
        const auto& vbView = vbPair.second;
        commandList->IASetVertexBuffers(vbSlot, 1, &vbView);
    }
}

void Renderer::Draw(UINT numOfVertices, UINT firstVertexToDraw)
{
    //pRenderEnv->ResetRenderCommandList();
    //pRenderEnv->SetupRenderingCommandList();

    auto commandList = pRenderEnv->GetRenderCommandList();

    CommonDraw(commandList.Get());

    commandList->DrawInstanced(numOfVertices, 1, firstVertexToDraw, 0);

    //pRenderEnv->ExecuteRenderCommandList();
}

void Renderer::DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation)
{
    //pRenderEnv->ResetRenderCommandList();
    //pRenderEnv->SetupRenderingCommandList();

    auto commandList = pRenderEnv->GetRenderCommandList();

    CommonDraw(commandList.Get());
    
    if (indexBufferView.BufferLocation != D3D12_INDEX_BUFFER_VIEW().BufferLocation)
    {
        commandList->IASetIndexBuffer(&indexBufferView);
    }

    commandList->DrawIndexedInstanced(indexCount, 1, firstIndex, baseVertexLocation, 0);

    //pRenderEnv->ExecuteRenderCommandList();
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
    resource.d12resource = resourceManager.CreateDx12Resource(&descr);
    ResourceManager::Resource::Id  resId = resourceManager.AddResource(resource);
    if (pSrcData != nullptr)
    {
        resourceManager.UpdateSRVBuffer(resId, pSrcData);
    }
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    pRenderEnv->ExecuteRenderCommandList();

    return resId;
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
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    if (indexSize == 4)
    {
        ibFormat = DXGI_FORMAT_R32_UINT;
    }
    else if (indexSize == 2)
    {
        ibFormat = DXGI_FORMAT_R16_UINT;
    }
    ASSERT(ibFormat != DXGI_FORMAT_UNKNOWN);
    resource.variant.ibView = resourceManager.CreateIndexBufferView(resource.d12resource.Get(), dataSize, ibFormat);
    resourceManager.UpdateResourceState(resource.d12resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    return resourceManager.AddResource(resource);
}

// UPDATE METHODS:

void Renderer::UpdateConstantBuffer(ResourceManager::Resource::Id resourceId, const void* pSrcData, const std::size_t bufferSize)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pSrcData, bufferSize, 0, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    resource.variant.cbHandle = resource.d12resource.Get()->GetGPUVirtualAddress();
}

void Renderer::UpdateTextureResource(ResourceManager::Resource::Id resourceId, D3D12_SUBRESOURCE_DATA* pSrcData)
{
    ASSERT(resourceId != 0);
    ASSERT(pSrcData != nullptr);
    resourceManager.UpdateSRVBuffer(resourceId, pSrcData, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::UpdateVertexBuffer(ResourceManager::Resource::Id resourceId, const void* pVertexData, const std::size_t numOfVertices, const std::size_t firstVertex, const std::size_t vertexSize)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    const std::size_t dataSize = vertexSize * numOfVertices;
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pVertexData, dataSize, firstVertex * vertexSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    resource.variant.vbView = resourceManager.CreateVertexBufferView(resource.d12resource.Get(), dataSize, vertexSize);
}

void Renderer::UpdateIndexBuffer(ResourceManager::Resource::Id resourceId, const void* pIndexData, const std::size_t numOfIndices, const std::size_t firstIndex, const std::size_t indexSize)
{
    const std::size_t dataSize = numOfIndices * indexSize;
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateBufferData(resource.d12resource.Get(), pIndexData, dataSize, firstIndex * indexSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
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
    ASSERT(slot <= ResourceManager::DESCR_HEAP_MAX);
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::SRV);
    srvArguments[slot] = resourceId;
}

void Renderer::BindVertexBuffer(UINT Slot, ResourceManager::Resource::Id resourceId, UINT Offset)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::VB);
    D3D12_VERTEX_BUFFER_VIEW vbView = resource.variant.vbView;
    if (Offset > 0)
    {
        vbView.SizeInBytes -= Offset;
        vbView.BufferLocation += Offset;
    }
    vertexBuffers[Slot] = vbView;
}

void Renderer::BindIndexBuffer(ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);
    ASSERT(resource.type == ResourceManager::Resource::Type::IB);
    indexBufferView = resource.variant.ibView;
}

// RELEASE:
void Renderer::ClearUploadBuffers()
{
    resourceManager.ClearUploadBuffers();
}

void Renderer::DropAllBindings()
{
    vertexBuffers.clear();
    srvArguments.clear();
    //cbArguments.clear();
    mappedSrv.clear();
    indexBufferView = {};
}

void Renderer::UnbindResource(ResourceManager::Resource::Id resourceId)
{
    ResourceManager::Resource resource = resourceManager.GetResource(resourceId);

    ASSERT(resource.type != ResourceManager::Resource::Type::UNDEFINED);
    
    bool found = false;
    Slot slot = 0;
    switch (resource.type)
    {
    // Unbind index buffer.
    case ResourceManager::Resource::Type::IB:
        if (indexBufferView.BufferLocation == resource.variant.ibView.BufferLocation)
        {
            indexBufferView = {};
        }
        break;

    // Unbind vertex buffer.
    case ResourceManager::Resource::Type::VB:
        for (const auto& pair : vertexBuffers)
        {
            if (pair.second.BufferLocation == resource.variant.vbView.BufferLocation)
            {
                found = true;
                slot = pair.first;
                break;
            }
        }
        if (found) vertexBuffers.erase(slot);
        break;

    // Unbind constant buffer.
    case ResourceManager::Resource::Type::CB:
        for (const auto& pair : cbArguments)
        {
            if (pair.second == resource.variant.cbHandle)
            {
                found = true;
                slot = pair.first;
                break;
            }
        }
        if (found) cbArguments.erase(slot);
        break;

    // Unbind texture buffer.
    case ResourceManager::Resource::Type::SRV:
        for (const auto& pair : srvArguments)
        {
            if (pair.second == resourceId)
            {
                found = true;
                slot = pair.first;
                break;
            }
        }
        if (found) srvArguments.erase(slot);
        if (found) mappedSrv.erase(slot);
        break;
    }
}

void Renderer::ReleaseResource(ResourceManager::Resource::Id resourceId)
{
    UnbindResource(resourceId);
    resourceManager.ReleaseResource(resourceId);
}

UINT Renderer::CreatePSO(const State* psoState)
{
    return stateManager.CreatePipelineStateObject(*psoState);
}

void Renderer::CreatePSO(const State* psoState, int stateId)
{
    stateManager.CreatePipelineStateObject(*psoState, stateId);
}

void Renderer::SetPSO(UINT PSOid)
{
    psoId = PSOid;
}

UINT Renderer::GetPSOiD()
{
    return psoId;
}

State Renderer::GetCurrentRenderState()
{
    return stateManager.GetStateDescr(psoId);
}