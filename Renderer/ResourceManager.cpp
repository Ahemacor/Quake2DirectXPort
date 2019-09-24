#include "ResourceManager.h"
#include "Utils.h"

void ResourceManager::Initialize(RenderEnvironment* pEnv)
{
    ASSERT(pEnv != nullptr);
    pRenderEnv = pEnv;
}

void ResourceManager::Release()
{
    pRenderEnv = nullptr;
    resources.clear();
    ClearUploadBuffers();
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateResource(const CD3DX12_RESOURCE_DESC* desc, const D3D12_RESOURCE_STATES origState, const D3D12_HEAP_TYPE heapType)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        desc,
        origState,
        nullptr,
        IID_PPV_ARGS(&resource)));
    resources.push_back(resource);
    return resource;
}

D3D12_VERTEX_BUFFER_VIEW ResourceManager::CreateVertexBufferView(ID3D12Resource* vertexBuffer, const std::size_t bufferSize, const std::size_t elementSize)
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = bufferSize;
    vertexBufferView.StrideInBytes = elementSize;
    return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW ResourceManager::CreateIndexBufferView(ID3D12Resource* indexBuffer, const std::size_t bufferSize, const DXGI_FORMAT format)
{
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = bufferSize;
    indexBufferView.Format = format;
    return indexBufferView;
}

void ResourceManager::UpdateResourceState(ID3D12Resource* resource, const D3D12_RESOURCE_STATES prev, const D3D12_RESOURCE_STATES next)
{
    auto commandList = pRenderEnv->GetGraphicsCommandList();
    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, prev, next);
    commandList->ResourceBarrier(1, &barrier);
}

void ResourceManager::UpdateVertexBuffer(ID3D12Resource* vertexBuffer, const void* pVertexData, const std::size_t dataSize)
{
    UpdateResourceState(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    uploadBuffers.push_back(uploadBuffer);

    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, pVertexData, dataSize);
    uploadBuffer->Unmap(0, nullptr);

    auto commandList = pRenderEnv->GetGraphicsCommandList();
    commandList->CopyBufferRegion(vertexBuffer, 0, uploadBuffer.Get(), 0, dataSize);

    UpdateResourceState(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void ResourceManager::UpdateIndexBuffer(ID3D12Resource* indexBuffer, const void* pIndexData, const std::size_t dataSize)
{
    UpdateResourceState(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    uploadBuffers.push_back(uploadBuffer);

    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, pIndexData, dataSize);
    uploadBuffer->Unmap(0, nullptr);

    auto commandList = pRenderEnv->GetGraphicsCommandList();
    commandList->CopyBufferRegion(indexBuffer, 0, uploadBuffer.Get(), 0, dataSize);

    UpdateResourceState(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

/*ID3D12DescriptorHeap* ResourceManager::GetDescriptorHeap()
{
    return descriptorHeap.Get();
}*/

void ResourceManager::ClearUploadBuffers()
{
    uploadBuffers.clear();
}