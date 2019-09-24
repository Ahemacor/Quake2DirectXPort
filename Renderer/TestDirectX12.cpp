#include "TestDirectX12.h"
#include "WindowsWindow.h"
#include "RenderEnvironment.h"
#include "Renderer.h"
#include "ImageIO/ImageIO.h"
#include <cassert>

struct Vertex
{
    float position[3];
    float uv[2];
};

static const Vertex vertices[4] = {
    // Upper Left
    { { -1.0f, 1.0f, 0 }, { 0, 0 } },
    // Upper Right
    { { 1.0f, 1.0f, 0 }, { 1, 0 } },
    // Bottom right
    { { 1.0f, -1.0f, 0 }, { 1, 1 } },
    // Bottom left
    { { -1.0f, -1.0f, 0 }, { 0, 1 } }
};

static const int indices[6] = {
    0, 1, 2, 2, 3, 0
};

const char* imageFilePath = "ruby.jpg";

HINSTANCE hInstance = nullptr;
WNDPROC winproc = nullptr;

WindowsWindow* g_window = nullptr;
RenderEnvironment* g_renderEnv = nullptr;
Renderer* g_renderer = nullptr;

static void TestInit()
{
    g_renderer->stateManager.SetVertexShader(PipelineStateManager::VS_Test);
    g_renderer->stateManager.SetPixelShader(PipelineStateManager::PS_Test);

    g_renderEnv->ResetCommandList();

    g_renderer->resourceManager.CreateVertexBuffer(vertices, sizeof(Vertex), ARRAYSIZE(vertices));
    g_renderer->resourceManager.CreateIndexBuffer(indices, ARRAYSIZE(indices));

    int width = 0, height = 0;
    std::vector<std::uint8_t> imageData = LoadImageFromFile(imageFilePath, 1, &width, &height);
    g_renderer->resourceManager.CreateSRVBuffer(imageData.data(), width, height);

    g_renderEnv->ExecuteCommandList();
}

static void TestRender()
{
    g_renderer->stateManager.RebuildState();

    auto commandList = g_renderEnv->GetGraphicsCommandList();
    commandList->SetPipelineState(g_renderer->stateManager.GetPSO());
    commandList->SetGraphicsRootSignature(g_renderer->stateManager.GetRootSignature());

    ID3D12DescriptorHeap* heaps[] = { g_renderer->resourceManager.GetDescriptorHeap(),
                                      g_renderer->stateManager.GetSamplerDescriptorHeap() };

    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSRV, g_renderer->resourceManager.GetSrvHandle(0));
    commandList->SetGraphicsRootDescriptorTable(PipelineStateManager::TextureSampler, g_renderer->stateManager.GetSamplerDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &(g_renderer->resourceManager.GetVertexBufferView(0)));
    commandList->IASetIndexBuffer(&g_renderer->resourceManager.GetIndexBufferView(0));
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    g_renderEnv->Synchronize();
}

void DX12_Init()
{
    assert(g_window == nullptr);
    g_window = new WindowsWindow;

    assert(g_renderEnv == nullptr);
    g_renderEnv = new RenderEnvironment;

    assert(g_renderer == nullptr);
    g_renderer = new Renderer;
}

void DX12_Release()
{
    if (g_window != nullptr)
    {
        g_window->Hide();
        delete g_window;
        g_window = nullptr;
    }

    if (g_renderEnv != nullptr)
    {
        g_renderEnv->Release();
        delete g_renderEnv;
        g_renderEnv = nullptr;
    }

    if (g_renderer != nullptr)
    {
        g_renderer->Release();
        delete g_renderer;
        g_renderer = nullptr;
    }
}

void DX12_Render_Begin()
{
    g_renderEnv->ClearScreen();
    TestRender();
}

void DX12_Render_End()
{
    g_renderEnv->Present();
}

void DX12_SetAppProps(HINSTANCE hInstance, WNDPROC winproc)
{
    assert(g_window != nullptr);
    g_window->SetParentInstance(hInstance);
    g_window->SetWindowProcedure(winproc);
}

bool DX12_InitWindow(int width, int height, int mode, bool fullscreen)
{
    RECT winrect = {};
    winrect.left = 0;
    winrect.top = 0;
    winrect.right = width;
    winrect.bottom = height;

    bool initSuccess = true;
    initSuccess = initSuccess && g_window->Show(winrect, fullscreen);
    initSuccess = initSuccess && g_renderEnv->Initialize(g_window->GetHandle(), width, height);
    initSuccess = initSuccess && g_renderer->Init(g_renderEnv);

    return initSuccess;
}

void DX12_CloseWindow()
{
    g_renderer->Release();
    g_renderEnv->Release();
    g_window->Hide();
}

void DX12_InitDefaultStates()
{
    TestInit();
}

UINT DX12_GetModesNumber()
{
    return g_renderEnv->GetVideoModesNumber();
}

DXGI_MODE_DESC DX12_GetVideoMode(UINT index)
{
    return g_renderEnv->GetVideoMode(index);
}

HWND DX12_GetOsWindowHandle()
{
    return g_window->GetHandle();
}

void DX12_SetPrimitiveTopologyTriangleList()
{
    //g_renderer->SetPrimitiveTopologyTriangleList();
}