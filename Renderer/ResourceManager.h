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
    void Initialize(RenderEnvironment* pEnv);
    void Release();

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(const CD3DX12_RESOURCE_DESC* desc,
                                                          const D3D12_RESOURCE_STATES origState = D3D12_RESOURCE_STATE_COPY_DEST,
                                                          const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* vertexBuffer, const std::size_t bufferSize, const std::size_t elementSize);
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* indexBuffer, const std::size_t bufferSize, const DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);

    void UpdateResourceState(ID3D12Resource* resource, const D3D12_RESOURCE_STATES prev, const D3D12_RESOURCE_STATES next);
    void UpdateVertexBuffer(ID3D12Resource* vertexBuffer, const void* pVertexData, const std::size_t dataSize);
    void UpdateIndexBuffer(ID3D12Resource* indexBuffer, const void* pIndexData, const std::size_t dataSize);

    //ID3D12DescriptorHeap* GetDescriptorHeap();

    void ClearUploadBuffers();

private:
    RenderEnvironment* pRenderEnv = nullptr;

    //Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    //D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    //Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    //D3D12_INDEX_BUFFER_VIEW indexBufferView;
    //Microsoft::WRL::ComPtr<ID3D12Resource> image;
    //std::vector<std::uint8_t> imageData;
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> uploadBuffers;

};

