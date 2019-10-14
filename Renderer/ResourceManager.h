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
            CD3DX12_RESOURCE_DESC texDescr; //D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
            D3D12_VERTEX_BUFFER_VIEW vbView;
            D3D12_INDEX_BUFFER_VIEW ibView;
            D3D12_BUFFER_SRV srvBuffer;
        } variant;

        Microsoft::WRL::ComPtr<ID3D12Resource>  d12resource;
    };

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
    D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceView(ResourceManager::Resource::Id resourceId, const std::size_t slot = 0);
    
    // Update RESOURCE content.
    void UpdateSRVBuffer(Resource::Id resourceId,
                         D3D12_SUBRESOURCE_DATA* pSrcData,
                         const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST);

    void UpdateBufferData(ID3D12Resource* resourceBuffer,
                          const void* pSrcData,
                          const std::size_t dataSize,
                          const std::size_t offset = 0,
                          const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST);


    void RebuildDescriptorHeap();
    ID3D12DescriptorHeap* GetDescriptorHeap();
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle();

    ID3D12Resource* CreateUploadBuffer(const std::size_t bufferSize);
    void ClearUploadBuffers();

private:
    std::unordered_map<Resource::Id, Resource> resourceMap;
    RenderEnvironment* pRenderEnv = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    //std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;
    std::unordered_map<std::size_t, Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;

};

