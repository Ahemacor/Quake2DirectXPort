#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"
#include <vector>
#include <string>

class ResourceManager
{
public:
    static const std::size_t DESCR_HEAP_MAX = 256;
    using ResourceId = std::size_t;

    void Initialize(RenderEnvironment* pEnv);
    void Release();

    // RESOURCE creation.
    ResourceId CreateResource(const CD3DX12_RESOURCE_DESC* desc,
                              const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST,
                              const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

    Microsoft::WRL::ComPtr<ID3D12Resource> GetResource(ResourceId resourceId);

    // RESOURCE state transition.
    void UpdateResourceState(ID3D12Resource* resource, const D3D12_RESOURCE_STATES prev, const D3D12_RESOURCE_STATES next);

    // Create RESOURCE view.
    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* vertexBuffer, const std::size_t bufferSize, const std::size_t elementSize);
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* indexBuffer, const std::size_t bufferSize, const DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateShaderResourceView(ResourceId& outSrvId, const std::size_t width, const std::size_t height);
    
    // Update RESOURCE content.
    void UpdateVertexBuffer(ID3D12Resource* vertexBuffer, const void* pVertexData, const std::size_t dataSize);
    void UpdateIndexBuffer(ID3D12Resource* indexBuffer, const void* pIndexData, const std::size_t dataSize);
    void UpdateSRVBuffer(ID3D12Resource* imageResource,
                         const void* pImageData,
                         const std::size_t width,
                         const std::size_t height,
                         const std::size_t texelSize = 4,
                         const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST);
    void UpdateBufferData(ID3D12Resource* resourceBuffer,
                          const void* pSrcData,
                          const std::size_t dataSize,
                          const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST);

    // GET/SET new RESOURCE. 
    ResourceId CreateVertexBuffer(const void* pVertexData, const std::size_t vertexSize, const std::size_t numOfVertices);
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(ResourceId viewId);

    ResourceId CreateIndexBuffer(const void* pIndexData, const std::size_t numOfIndices, const DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT, const std::size_t indexSize = 4);
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(ResourceId indexId);

    ResourceId CreateSRVBuffer(const void* pImageData, const std::size_t width, const std::size_t height);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(ResourceId srvId);

    ResourceId CreateConstantBuffer(const void* pSrcData, const std::size_t bufferSize);
    D3D12_GPU_VIRTUAL_ADDRESS GetCBHandle(ResourceId srvId);


    ID3D12DescriptorHeap* GetDescriptorHeap();
    void ClearUploadBuffers();

private:
    RenderEnvironment* pRenderEnv = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferViews;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
    std::size_t numDescr = 0;

};

