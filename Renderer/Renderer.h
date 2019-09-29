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

class ScopedStateManager
{
public:
    ScopedStateManager(bool isScoped, PipelineStateManager& sm, RenderEnvironment& re)
        : isScoped(isScoped)
        , stateManager(sm)
        , renderEnv(re)
    {
        if(isScoped) renderEnv.ResetCommandList();
    }

    ~ScopedStateManager()
    {
        if (isScoped) renderEnv.ExecuteCommandList();
    }

    PipelineStateManager* operator->()
    {
        return &stateManager;
    }

private:
    const bool isScoped;
    PipelineStateManager& stateManager;
    RenderEnvironment& renderEnv;
};


class Renderer
{
public:
    using Slot = std::size_t;

    Renderer();
    ~Renderer();

    bool Init(RenderEnvironment* environment);
    void Release();

    void Draw(UINT numOfVertices, UINT firstVertexToDraw = 0);
    void DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation);

    // CREATE METHODS:
    ResourceManager::Resource::Id CreateConstantBuffer(const std::size_t bufferSize, const void* pSrcData = nullptr);
    ResourceManager::Resource::Id CreateTextureResource(const std::size_t width, const std::size_t height, const void* pImageData = nullptr);
    ResourceManager::Resource::Id CreateVertexBuffer(const std::size_t numOfVertices, const std::size_t vertexSize, const void* pVertexData = nullptr);
    ResourceManager::Resource::Id CreateIndexBuffer(const std::size_t numOfIndices, const void* pIndexData = nullptr, const std::size_t indexSize = sizeof(DWORD));

    // UPDATE METHODS:
    void UpdateConstantBuffer(ResourceManager::Resource::Id resourceId, const void* pSrcData, const std::size_t bufferSize);
    void UpdateTextureResource(ResourceManager::Resource::Id resourceId, const void* pImageData, const std::size_t width, const std::size_t height);
    void UpdateVertexBuffer(ResourceManager::Resource::Id resourceId, const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize);
    void UpdateIndexBuffer(ResourceManager::Resource::Id resourceId, const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize = sizeof(DWORD));

    // BIND METHODS:
    void BindConstantBuffer(ResourceManager::Resource::Id resourceId, std::size_t slot);
    void BindTextureResource(ResourceManager::Resource::Id resourceId, std::size_t slot);
    void BindVertexBuffer(UINT Slot, ResourceManager::Resource::Id resourceId);
    void BindIndexBuffer(ResourceManager::Resource::Id resourceId);

    ScopedStateManager GetStateManager(bool isScoped = false) { return ScopedStateManager(isScoped, stateManager, *pRenderEnv); }

    void SetPrimitiveTopologyTriangleList();

    const State& GetRenderState();
    void SetRenderState(const State& state);

private:
    struct VertexBufferToBind
    {
        UINT slot;
        D3D12_VERTEX_BUFFER_VIEW view = {};
    } vertexBufferToBind;

    RenderEnvironment* pRenderEnv = nullptr;
    PipelineStateManager stateManager;
    ResourceManager resourceManager;

    std::map<Slot, D3D12_GPU_VIRTUAL_ADDRESS> cbArguments;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    bool isInitialized = false;

};
