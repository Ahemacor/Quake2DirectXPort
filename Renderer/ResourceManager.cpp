#include "ResourceManager.h"
#include "Utils.h"

void ResourceManager::Initialize(RenderEnvironment* pEnv)
{
    ASSERT(pEnv != nullptr);
    pRenderEnv = pEnv;
    descriptorHeap = pRenderEnv->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DESCR_HEAP_MAX, true);
}

void ResourceManager::Release()
{
    pRenderEnv = nullptr;
    resources.clear();
    vertexBufferViews.clear();
    indexBufferViews.clear();
    descriptorHeap.Reset();
    ClearUploadBuffers();
    numDescr = 0;
}

ResourceManager::ResourceId ResourceManager::CreateResource(const CD3DX12_RESOURCE_DESC* desc, const D3D12_RESOURCE_STATES origState, const D3D12_HEAP_TYPE heapType)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    ENSURE_RESULT(pRenderEnv->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        desc,
        origState,
        nullptr,
        IID_PPV_ARGS(&resource)));
    resources.push_back(resource);
    return (resources.size() - 1);
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::GetResource(ResourceId resourceId)
{
    return resources[resourceId];
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

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateShaderResourceView(ResourceId& outSrvId, const std::size_t width, const std::size_t height)
{
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width, height, 1, 1);
    const ResourceId resId = CreateResource(&resourceDesc);
    Microsoft::WRL::ComPtr<ID3D12Resource> imageBuffer = GetResource(resId);

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    UINT descrHandleSize = pRenderEnv->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    cpuHandle.Offset(descrHandleSize * numDescr);
    outSrvId = numDescr++;

    pRenderEnv->GetDevice()->CreateShaderResourceView(imageBuffer.Get(),
                                                      &shaderResourceViewDesc,
                                                      cpuHandle);
    return imageBuffer;
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
    const ResourceId resId = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = GetResource(resId);
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
    const ResourceId resId = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = GetResource(resId);
    uploadBuffers.push_back(uploadBuffer);

    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, pIndexData, dataSize);
    uploadBuffer->Unmap(0, nullptr);

    auto commandList = pRenderEnv->GetGraphicsCommandList();
    commandList->CopyBufferRegion(indexBuffer, 0, uploadBuffer.Get(), 0, dataSize);

    UpdateResourceState(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
}

void ResourceManager::UpdateSRVBuffer(ID3D12Resource* imageResource, const void* pImageData, const std::size_t width, const std::size_t height, const std::size_t texelSize, const D3D12_RESOURCE_STATES origState)
{
    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(imageResource, origState, D3D12_RESOURCE_STATE_COPY_DEST);

    const auto uploadBufferSize = GetRequiredIntermediateSize(imageResource, 0, 1);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    const ResourceId resId = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = GetResource(resId);

    D3D12_SUBRESOURCE_DATA srcData;
    srcData.pData = pImageData;
    srcData.RowPitch = width * texelSize;
    srcData.SlicePitch = width * height * texelSize;

    auto uploadCommandList = pRenderEnv->GetGraphicsCommandList();
    UpdateSubresources(uploadCommandList.Get(), imageResource, uploadBuffer.Get(), 0, 0, 1, &srcData);

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(imageResource, D3D12_RESOURCE_STATE_COPY_DEST, origState);
}

void ResourceManager::UpdateBufferData(ID3D12Resource* resourceBuffer, const void* pSrcData, const std::size_t dataSize, const D3D12_RESOURCE_STATES origState)
{
    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(resourceBuffer, origState, D3D12_RESOURCE_STATE_COPY_DEST);

    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    const ResourceId resId = CreateResource(&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer = GetResource(resId);
    uploadBuffers.push_back(uploadBuffer);

    void* p;
    uploadBuffer->Map(0, nullptr, &p);
    ::memcpy(p, pSrcData, dataSize);
    uploadBuffer->Unmap(0, nullptr);

    auto commandList = pRenderEnv->GetGraphicsCommandList();
    commandList->CopyBufferRegion(resourceBuffer, 0, uploadBuffer.Get(), 0, dataSize);

    if (origState != D3D12_RESOURCE_STATE_COPY_DEST)
        UpdateResourceState(resourceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, origState);
}

ResourceManager::ResourceId ResourceManager::CreateVertexBuffer(const void* pVertexData, const std::size_t vertexSize, const std::size_t numOfVertices)
{
    const std::size_t dataSize = vertexSize * numOfVertices;
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    const ResourceId resId = CreateResource(&vertexBufferDesc);
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer = GetResource(resId);
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = CreateVertexBufferView(vertexBuffer.Get(), dataSize, vertexSize);
    UpdateBufferData(vertexBuffer.Get(), pVertexData, dataSize, D3D12_RESOURCE_STATE_COPY_DEST);
    UpdateResourceState(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    vertexBufferViews.push_back(vertexBufferView);
    return (vertexBufferViews.size() - 1);
}

D3D12_VERTEX_BUFFER_VIEW ResourceManager::GetVertexBufferView(ResourceId viewId)
{
    return vertexBufferViews[viewId];
}

ResourceManager::ResourceId ResourceManager::CreateIndexBuffer(const void* pIndexData, const std::size_t numOfIndices, const DXGI_FORMAT indexFormat, const std::size_t indexSize)
{
    const std::size_t dataSize = numOfIndices * indexSize;
    CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    const ResourceId resId = CreateResource(&indexBufferDesc);
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer = GetResource(resId);
    D3D12_INDEX_BUFFER_VIEW indexBufferView = CreateIndexBufferView(indexBuffer.Get(), dataSize);
    UpdateBufferData(indexBuffer.Get(), pIndexData, dataSize, D3D12_RESOURCE_STATE_COPY_DEST);
    UpdateResourceState(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    indexBufferViews.push_back(indexBufferView);
    return (indexBufferViews.size() - 1);
}

D3D12_INDEX_BUFFER_VIEW ResourceManager::GetIndexBufferView(ResourceId indexId)
{
    return indexBufferViews[indexId];
}

ResourceManager::ResourceId ResourceManager::CreateSRVBuffer(const void* pImageData, const std::size_t width, const std::size_t height)
{
    ResourceId srvId = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> imageBuffer = CreateShaderResourceView(srvId, width, height);
    UpdateSRVBuffer(imageBuffer.Get(), pImageData, width, height);
    UpdateResourceState(imageBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    return srvId;
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::GetSrvHandle(ResourceId srvId)
{
    const auto descriptorOffset = pRenderEnv->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescrHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, descriptorOffset);
    gpuDescrHandle.Offset(srvId, descriptorOffset);
    return gpuDescrHandle;
}

ResourceManager::ResourceId ResourceManager::CreateConstantBuffer(const void* pSrcData, const std::size_t bufferSize)
{
    const CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    const ResourceId resId = CreateResource(&descr);
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer = GetResource(resId);
    UpdateBufferData(constantBuffer.Get(), pSrcData, bufferSize, D3D12_RESOURCE_STATE_COPY_DEST);
    UpdateResourceState(constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    return resId;
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceManager::GetCBHandle(ResourceId cbId)
{
    return GetResource(cbId)->GetGPUVirtualAddress();
}

ID3D12DescriptorHeap* ResourceManager::GetDescriptorHeap()
{
    return descriptorHeap.Get();
}

void ResourceManager::ClearUploadBuffers()
{
    uploadBuffers.clear();
}
