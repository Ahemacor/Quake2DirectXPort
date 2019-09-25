#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "RenderEnvironment.h"
#include "PipelineStateManager.h"
#include "ResourceManager.h"
#include <map>
#include <string>

class Renderer
{
public:
    using Slot = std::size_t;

    Renderer();
    ~Renderer();

    bool Init(RenderEnvironment* environment);
    void Release();

    void Draw(UINT numOfVertices, UINT firstVertexToDraw = 0);
    void Draw(UINT indexCount, UINT firstIndex, UINT baseVertexLocation);

    // REGISTER METHODS:
    ResourceManager::ResourceId RegisterConstantBuffer(const std::size_t bufferSize);
    //ResourceManager::ResourceId RegisterTextureResource(const std::size_t width, const std::size_t height);
    ResourceManager::ResourceId RegisterVertexBuffer(const std::size_t numOfVertices, const std::size_t vertexSize);
    ResourceManager::ResourceId RegisterIndexBuffer(const std::size_t numOfIndices, const std::size_t indexSize = sizeof(DWORD));

    // CREATE METHODS:
    ResourceManager::ResourceId CreateConstantBuffer(const void* pSrcData, const std::size_t bufferSize);
    ResourceManager::ResourceId CreateTextureResource(const void* pImageData, const std::size_t width, const std::size_t height);
    ResourceManager::ResourceId CreateVertexBuffer(const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize);
    ResourceManager::ResourceId CreateIndexBuffer(const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize = sizeof(DWORD));

    // UPDATE METHODS:
    void UpdateConstantBuffer(ResourceManager::ResourceId resourceId, const void* pSrcData, const std::size_t bufferSize);
    //void UpdateTextureResource(ResourceManager::ResourceId resourceId, const void* pImageData, const std::size_t width, const std::size_t height);
    void UpdateVertexBuffer(ResourceManager::ResourceId resourceId, const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize);
    void UpdateIndexBuffer(ResourceManager::ResourceId resourceId, const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize = sizeof(DWORD));

    // BIND METHODS:
    void BindConstantBuffer(ResourceManager::ResourceId resourceId, std::size_t slot);
    void BindTextureResource(ResourceManager::ResourceId resourceId, std::size_t slot);
    void BindVertexBuffer(ResourceManager::ResourceId resourceId);
    void BindIndexBuffer(ResourceManager::ResourceId resourceId);

    // UNBIND METHODS:
    void UnbindConstantBuffer(std::size_t slot);
    void UnbindTextureResource(std::size_t slot);

    void SetPrimitiveTopology(PipelineStateManager::PrimitiveTopologyType topology = PipelineStateManager::PrimitiveTopologyType::TriangleList);

private:
    RenderEnvironment* pRenderEnv = nullptr;
    PipelineStateManager stateManager;
    ResourceManager resourceManager;

    std::map<Slot, D3D12_GPU_VIRTUAL_ADDRESS> cbArguments;
    std::map<Slot, D3D12_GPU_DESCRIPTOR_HANDLE> srvArguments;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    bool isInitialized = false;

};
