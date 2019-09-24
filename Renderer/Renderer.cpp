#include "Renderer.h"
#include "Utils.h"
#include "ImageIO/ImageIO.h"
#include <cassert>



struct Vertex
{
    float position[3];
    float uv[2];
};

// Declare upload buffer data as 'static' so it persists after returning from this function.
// Otherwise, we would need to explicitly wait for the GPU to copy data from the upload buffer
// to vertex/index default buffers due to how the GPU processes commands asynchronously. 
static const Vertex vertices[4] = {
    // Upper Left
    { { -1.0f, 1.0f, 0 },{ 0, 0 } },
    // Upper Right
    { { 1.0f, 1.0f, 0 },{ 1, 0 } },
    // Bottom right
    { { 1.0f, -1.0f, 0 },{ 1, 1 } },
    // Bottom left
    { { -1.0f, -1.0f, 0 },{ 0, 1 } }
};

static const int indices[6] = {
    0, 1, 2, 2, 3, 0
};

Renderer::Renderer() {}

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Init(RenderEnvironment* environment)
{
    assert(environment != nullptr);
    pRenderEnv = environment;

    stateManager.SetVertexShader(PipelineStateManager::VS_Test);
    stateManager.SetPixelShader(PipelineStateManager::PS_Test);
    stateManager.Initialize(pRenderEnv->GetDevice());
    resourceManager.Initialize(pRenderEnv);

    pRenderEnv->ResetCommandList();

    resourceManager.CreateVertexBuffer(vertices, sizeof(Vertex), ARRAYSIZE(vertices));
    resourceManager.CreateIndexBuffer(indices, ARRAYSIZE(indices));

    //CreateTexture();
    int width = 0, height = 0;
    const char* imageFilePath = "ruby.jpg";
    std::vector<std::uint8_t> imageData = LoadImageFromFile(imageFilePath, 1, &width, &height);
    resourceManager.CreateSRVBuffer(imageData.data(), width, height);
    
    pRenderEnv->ExecuteCommandList();
            
    isInitialized = true;
    return isInitialized;
}

void Renderer::Release()
{
    pRenderEnv = nullptr;

    stateManager.Release();
    resourceManager.Release();

    isInitialized = false;
}

void Renderer::RenderImpl()
{
    assert(isInitialized == true);

    auto commandList = pRenderEnv->GetGraphicsCommandList();

    stateManager.RebuildState();
    commandList->SetPipelineState(stateManager.GetPSO());
    commandList->SetGraphicsRootSignature(stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { resourceManager.GetDescriptorHeap(), stateManager.GetSamplerDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSRV, resourceManager.GetSrvHandle(0));

    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSampler, stateManager.GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &(resourceManager.GetVertexBufferView(0)));
    commandList->IASetIndexBuffer(&resourceManager.GetIndexBufferView(0));
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    pRenderEnv->Synchronize();
}
