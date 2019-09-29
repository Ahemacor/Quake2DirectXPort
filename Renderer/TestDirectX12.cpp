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

struct ConstantBuffer { float x, y, z, w; };

static const ConstantBuffer cb = { 0, 0, 0, 0 };

static float shortCb = 0.9f;
int cbId = -1;

HINSTANCE hInstance = nullptr;
WNDPROC winproc = nullptr;

WindowsWindow* g_window = nullptr;
RenderEnvironment* g_renderEnv = nullptr;
Renderer* g_renderer = nullptr;

int DX12_CreateConstantBuffer(const void* pSrcData, int bufferSize);
void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize);
void DX12_BindConstantBuffer(int resourceId, int slot);

static void TestInit()
{
    int width = 0, height = 0;
    std::vector<std::uint8_t> imageData = LoadImageFromFile(imageFilePath, 1, &width, &height);
    
    int cbId = DX12_CreateConstantBuffer(nullptr, sizeof(shortCb));
    DX12_UpdateConstantBuffer(cbId, &shortCb, sizeof(shortCb));
    DX12_BindConstantBuffer(cbId, 0);

    ScopedStateManager SM = g_renderer->GetStateManager();
    SM->SetVertexShader(PipelineStateManager::VS_Test);
    SM->SetPixelShader(PipelineStateManager::PS_Test);

    ResourceManager::Resource::Id srvId = g_renderer->CreateTextureResource(width, height, imageData.data());
    g_renderer->BindTextureResource(srvId, 0);

    ResourceManager::Resource::Id vbId = g_renderer->CreateVertexBuffer(ARRAYSIZE(vertices), sizeof(Vertex), vertices);
    g_renderer->BindVertexBuffer(vbId);

    ResourceManager::Resource::Id ibId = g_renderer->CreateIndexBuffer(ARRAYSIZE(indices), indices);
    g_renderer->BindIndexBuffer(ibId);
}

static void TestRender()
{
    g_renderer->DrawIndexed(6, 0, 0);
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

int DX12_InitWindow(int width, int height, int mode, int fullscreen)
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
    g_renderer->SetPrimitiveTopologyTriangleList();
}

int DX12_CreateConstantBuffer(const void* pSrcData, int bufferSize)
{
    ScopedStateManager SM = g_renderer->GetStateManager();
    return g_renderer->CreateConstantBuffer(bufferSize, pSrcData);
}

void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize)
{
    ScopedStateManager SM = g_renderer->GetStateManager();
    g_renderer->UpdateConstantBuffer(resourceId, pSrcData, bufferSize);
}

void DX12_BindConstantBuffer(int resourceId, int slot)
{
    g_renderer->BindConstantBuffer(resourceId, slot);
}