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

    isInitialized = true;
    NOT_IMPL_FAIL();
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();
    resourceManager.Release();
    NOT_IMPL_FAIL();
    isInitialized = false;
}

void Renderer::Draw(UINT numOfVertices, UINT firstVertexToDraw = 0)
{
    NOT_IMPL_FAIL();
}

void Renderer::Draw(UINT indexCount, UINT firstIndex, UINT baseVertexLocation)
{
    NOT_IMPL_FAIL();
}

// REGISTER METHODS:

ResourceManager::ResourceId Renderer::RegisterConstantBuffer(const std::size_t bufferSize)
{
    const CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    return resourceManager.CreateResource(&descr);
}

/*ResourceManager::ResourceId Renderer::RegisterTextureResource(const std::size_t width, const std::size_t height)
{
    NOT_IMPL_FAIL();
    return 0;
}*/

ResourceManager::ResourceId Renderer::RegisterVertexBuffer(const std::size_t numOfVertices, const std::size_t vertexSize)
{
    const CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(numOfVertices * vertexSize);
    return resourceManager.CreateResource(&descr);
}

ResourceManager::ResourceId Renderer::RegisterIndexBuffer(const std::size_t numOfIndices, const std::size_t indexSize)
{
    const CD3DX12_RESOURCE_DESC descr = CD3DX12_RESOURCE_DESC::Buffer(numOfIndices * indexSize);
    return resourceManager.CreateResource(&descr);
}

// CREATE METHODS:

ResourceManager::ResourceId Renderer::CreateConstantBuffer(const void* pSrcData, const std::size_t bufferSize)
{
    ResourceManager::ResourceId resourceId = RegisterConstantBuffer(bufferSize);
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = resourceManager.GetResource(resourceId);
    UpdateConstantBuffer(resourceId, pSrcData, bufferSize);
    resourceManager.UpdateResourceState(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    return resourceId;
}

ResourceManager::ResourceId Renderer::CreateTextureResource(const void* pImageData, const std::size_t width, const std::size_t height)
{
    ResourceManager::ResourceId srvId = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> imageBuffer = resourceManager.CreateShaderResourceView(srvId, width, height);
    resourceManager.UpdateSRVBuffer(imageBuffer.Get(), pImageData, width, height);
    resourceManager.UpdateResourceState(imageBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    return srvId;
}

ResourceManager::ResourceId Renderer::CreateVertexBuffer(const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize)
{
    NOT_IMPL_FAIL();
    return 0;
}

ResourceManager::ResourceId Renderer::CreateIndexBuffer(const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize)
{
    NOT_IMPL_FAIL();
    return 0;
}

// UPDATE METHODS:

void Renderer::UpdateConstantBuffer(ResourceManager::ResourceId resourceId, const void* pSrcData, const std::size_t bufferSize)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateBufferData(resource.Get(), pSrcData, bufferSize);
}

/*void Renderer::UpdateTextureResource(ResourceManager::ResourceId resourceId, const void* pImageData, const std::size_t width, const std::size_t height)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = resourceManager.GetResource(resourceId);
    resourceManager.UpdateSRVBuffer(resource.Get(), pImageData, width, height);
}*/

void Renderer::UpdateVertexBuffer(ResourceManager::ResourceId resourceId, const void* pVertexData, const std::size_t numOfVertices, const std::size_t vertexSize)
{
    NOT_IMPL_FAIL();
}

void Renderer::UpdateIndexBuffer(ResourceManager::ResourceId resourceId, const void* pIndexData, const std::size_t numOfIndices, const std::size_t indexSize)
{
    NOT_IMPL_FAIL();
}

// BIND METHODS:

void Renderer::BindConstantBuffer(ResourceManager::ResourceId resourceId, std::size_t slot)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = resourceManager.GetResource(resourceId);
    cbArguments[slot] = resource->GetGPUVirtualAddress();
}

void Renderer::BindTextureResource(ResourceManager::ResourceId srvId, std::size_t slot)
{
    srvArguments[slot] = resourceManager.GetSrvHandle(srvId);
}

void Renderer::BindVertexBuffer(ResourceManager::ResourceId resourceId)
{
    NOT_IMPL_FAIL();
}

void Renderer::BindIndexBuffer(ResourceManager::ResourceId resourceId)
{
    NOT_IMPL_FAIL();
}

// UNBIND METHODS:

void Renderer::UnbindConstantBuffer(std::size_t slot)
{
    auto it = cbArguments.find(slot);
    if (it != cbArguments.cend())
    {
        cbArguments.erase(it);
    }
}

void Renderer::UnbindTextureResource(std::size_t slot)
{
    NOT_IMPL_FAIL();
}

void Renderer::SetPrimitiveTopology(PipelineStateManager::PrimitiveTopologyType topology = PipelineStateManager::PrimitiveTopologyType::TriangleList)
{
    NOT_IMPL_FAIL();
}