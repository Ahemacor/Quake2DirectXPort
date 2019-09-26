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
#include <unordered_map>

class ResourceManager
{
public:
    struct Resource
    {
        using Id = std::size_t;
        enum Type
        {
            UNDEFINED = 0,
            CB,
            SRV,
            VB,
            IB
        } type = Type::UNDEFINED;

        union
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
            D3D12_VERTEX_BUFFER_VIEW vbView;
            D3D12_INDEX_BUFFER_VIEW ibView;
        } variant;

        Microsoft::WRL::ComPtr<ID3D12Resource>  d12resource;
    };

    std::unordered_map<Resource::Id, Resource> resourceMap;

    static const std::size_t DESCR_HEAP_MAX = 256;

    void Initialize(RenderEnvironment* pEnv);
    void Release();

    // RESOURCE Creation.
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDx12Resource(const CD3DX12_RESOURCE_DESC* desc,
                                                              const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST,
                                                              const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
    Resource::Id AddResource(const Resource& newResource);
    void ReleaseResource(Resource::Id resourceId);
    Resource GetResource(Resource::Id resourceId);

    // RESOURCE state transition.
    void UpdateResourceState(ID3D12Resource* resource, const D3D12_RESOURCE_STATES prev, const D3D12_RESOURCE_STATES next);

    // Create RESOURCE view.
    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* vertexBuffer, const std::size_t bufferSize, const std::size_t elementSize);
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* indexBuffer, const std::size_t bufferSize, const DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);
    D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ID3D12Resource* imageBuffer, const std::size_t width, const std::size_t height);
    
    // Update RESOURCE content.
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


    void RebuildDescriptorHeap();
    ID3D12DescriptorHeap* GetDescriptorHeap();
    std::size_t lastDescriptorIndex = 0;

    ID3D12Resource* CreateUploadBuffer(const std::size_t bufferSize);
    void ClearUploadBuffers();

private:
    RenderEnvironment* pRenderEnv = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;

};

