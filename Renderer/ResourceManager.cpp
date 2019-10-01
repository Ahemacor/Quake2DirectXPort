#include "ResourceManager.h"
#include "Utils.h"

static inline ResourceManager::Resource::Id NewId()
{
    static ResourceManager::Resource::Id id = 1;
    return id++;
}

void ResourceManager::Initialize(RenderEnvironment* pEnv)
{
    ASSERT(pEnv != nullptr);
    pRenderEnv = pEnv;
    RebuildDescriptorHeap();
}

void ResourceManager::Release()
{
    pRenderEnv = nullptr;
    resourceMap.clear();
    descriptorHeap.Reset();
    ClearUploadBuffers();
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateDx12Resource(const CD3DX12_RESOURCE_DESC* desc, const D3D12_RESOURCE_STATES origState, const D3D12_HEAP_TYPE heapType)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> dx12resource;
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        desc,
        origState,
        nullptr,
        IID_PPV_ARGS(&dx12resource)));
    return dx12resource;
}

ResourceManager::Resource::Id ResourceManager::AddResource(const Resource& newResource)
{
    ResourceManager::Resource::Id resourceId = NewId();
    ASSERT(resourceMap.find(resourceId) == resourceMap.cend());
    resourceMap[resourceId] = newResource;
    return resourceId;
}

void ResourceManager::ReleaseResource(Resource::Id resourceId)
{
    ASSERT(resourceMap.find(resourceId) != resourceMap.cend());
    resourceMap.erase(resourceId);
}

ResourceManager::Resource ResourceManager::GetResource(Resource::Id resourceId)
{
    ASSERT(resourceMap.find(resourceId) != resourceMap.cend());
    return resourceMap[resourceId];
}

void ResourceManager::UpdateResourceState(ID3D12Resource* resource, const D3D12_RESOURCE_STATES prev, const D3D12_RESOURCE_STATES next)
{
    pRenderEnv->ResetUpdateCommandList();
    auto commandList = pRenderEnv->GetUpdateCommandList();
    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, prev, next);
    commandList->ResourceBarrier(1, &barrier);
    pRenderEnv->ExecuteUpdateCommandList();
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

D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::CreateShaderResourceView(ResourceManager::Resource::Id resourceId, const std::size_t slot)
{
    Resource resource = GetResource(resourceId);
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = resource.variant.texDescr.Format;
    shaderResourceViewDesc.Texture2D.MipLevels = resource.variant.texDescr.MipLevels;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    const UINT descrHandleSize = pRenderEnv->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), slot, descrHandleSize);

    pRenderEnv->GetDevice()->CreateShaderResourceView(resource.d12resource.Get(), &shaderResourceViewDesc, cpuHandle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescrHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), slot, descrHandleSize);

    return gpuDescrHandle;
}

void ResourceManager::UpdateSRVBuffer(ID3D12Resource* imageResource, D3D12_SUBRESOURCE_DATA* pSrcData, const D3D12_RESOURCE_STATES origState)
{
    ASSERT(imageResource != nullptr);

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(imageResource, origState, D3D12_RESOURCE_STATE_COPY_DEST);

    const auto uploadBufferSize = GetRequiredIntermediateSize(imageResource, 0, 1);
    ID3D12Resource* uploadBuffer = CreateUploadBuffer(uploadBufferSize);

    /*D3D12_SUBRESOURCE_DATA srcData;
    srcData.pData = pImageData;
    srcData.RowPitch = width * texelSize;
    srcData.SlicePitch = width * height * texelSize;*/

    pRenderEnv->ResetUpdateCommandList();
    auto uploadCommandList = pRenderEnv->GetUpdateCommandList();
    //UpdateSubresources(uploadCommandList.Get(), imageResource, uploadBuffer, 0, 0, 1, &srcData);
    UpdateSubresources(uploadCommandList.Get(), imageResource, uploadBuffer, 0, 0, 0, pSrcData);
    pRenderEnv->ExecuteUpdateCommandList();

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(imageResource, D3D12_RESOURCE_STATE_COPY_DEST, origState);
}

void ResourceManager::UpdateBufferData(ID3D12Resource* resourceBuffer, const void* pSrcData, const std::size_t dataSize, const D3D12_RESOURCE_STATES origState)
{
    ASSERT(resourceBuffer != nullptr);
    ASSERT(pSrcData != nullptr);

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(resourceBuffer, origState, D3D12_RESOURCE_STATE_COPY_DEST);

    ID3D12Resource* uploadBuffer = CreateUploadBuffer(dataSize);

    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, pSrcData, dataSize);
    uploadBuffer->Unmap(0, nullptr);

    pRenderEnv->ResetUpdateCommandList();
    auto commandList = pRenderEnv->GetUpdateCommandList();
    commandList->CopyBufferRegion(resourceBuffer, 0, uploadBuffer, 0, dataSize);
    pRenderEnv->ExecuteUpdateCommandList();

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(resourceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, origState);
}

void ResourceManager::RebuildDescriptorHeap()
{
    descriptorHeap = pRenderEnv->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DESCR_HEAP_MAX, true);
}

ID3D12DescriptorHeap* ResourceManager::GetDescriptorHeap()
{
    return descriptorHeap.Get();
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::GetSrvHandle()
{
    return descriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
}

ID3D12Resource* ResourceManager::CreateUploadBuffer(const std::size_t bufferSize)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                                    D3D12_HEAP_FLAG_NONE,
                                                                    &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                    nullptr,
                                                                    IID_PPV_ARGS(&uploadResource)));
    uploadBuffers.push_back(uploadResource);
    return uploadResource.Get();
}

void ResourceManager::ClearUploadBuffers()
{
    uploadBuffers.clear();
}
