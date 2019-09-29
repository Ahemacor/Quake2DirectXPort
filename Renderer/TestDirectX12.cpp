#include "TestDirectX12.h"
#include "WindowsWindow.h"
#include "RenderEnvironment.h"
#include "Renderer.h"
#include "ImageIO/ImageIO.h"
#include <cassert>

struct Vertex
{
    float position[2];
    float uv[3];
};

static const Vertex vertices[4] = {
    // Upper Left
    { { -1.0f, 1.0f }, { 0, 0, 0 } },
    // Upper Right
    { { 1.0f, 1.0f }, { 1, 0, 0 } },
    // Bottom right
    { { 1.0f, -1.0f }, { 1, 1, 0 } },
    // Bottom left
    { { -1.0f, -1.0f }, { 0, 1, 0 } }
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

    ScopedStateManager SM = g_renderer->GetStateManager(true);
    SM->SetInputLayout(InputLayout::INPUT_LAYOUT_TEXARRAY);
    SM->SetVertexShader(ShaderType::SHADER_TEST_VS);
    SM->SetPixelShader(ShaderType::SHADER_TEST_PS);
    SM->SetBlendState(BlendState::BSNone);
    SM->SetDepthState(DepthStencilState::DSDepthNoWrite);
    SM->SetRasterizerState(RasterizerState::RSNoCull);
    SM->SetPrimitiveTopology();

    ResourceManager::Resource::Id srvId = g_renderer->CreateTextureResource(width, height, imageData.data());
    g_renderer->BindTextureResource(srvId, 0);

    ResourceManager::Resource::Id vbId = g_renderer->CreateVertexBuffer(ARRAYSIZE(vertices), sizeof(Vertex), vertices);
    DX12_BindVertexBuffer(0, vbId);

    ResourceManager::Resource::Id ibId = g_renderer->CreateIndexBuffer(ARRAYSIZE(indices), indices);
    g_renderer->BindIndexBuffer(ibId);
}

static void TestRender()
{
    DX12_DrawIndexed(6, 0, 0);
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
    //g_renderEnv->ClearScreen();
    //TestRender();
}

void DX12_Render_End()
{
    //g_renderEnv->Present();
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
    //TestInit();
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
    ScopedStateManager SM = g_renderer->GetStateManager(true);
    return g_renderer->CreateConstantBuffer(bufferSize, pSrcData);
}

void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize)
{
    ScopedStateManager SM = g_renderer->GetStateManager(true);
    g_renderer->UpdateConstantBuffer(resourceId, pSrcData, bufferSize);
}

void DX12_BindConstantBuffer(int resourceId, int slot)
{
    g_renderer->BindConstantBuffer(resourceId, slot);
}

void DX12_BindVertexBuffer(UINT Slot, int resourceId)
{
    g_renderer->BindVertexBuffer(Slot, resourceId);
}

void DX12_SetViewport(const D3D12_VIEWPORT* pViewport)
{
    g_renderEnv->SetViewport(*pViewport);
}

void DX12_Draw(UINT numOfVertices, UINT firstVertexToDraw)
{
    g_renderer->Draw(numOfVertices, firstVertexToDraw);
}

void DX12_DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation)
{
    g_renderer->DrawIndexed(indexCount, firstIndex, baseVertexLocation);
}

const State* Dx12_GetRenderState()
{
    return &g_renderer->GetRenderState();
}

void Dx12_SetRenderState(const State* state)
{
    g_renderer->SetRenderState(*state);
}

void DX12_SetVertexShader(ShaderType shaderType)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetVertexShader(shaderType);
}

void DX12_SetPixelShader(ShaderType shaderType)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetPixelShader(shaderType);
}

void DX12_SetInputLayout(InputLayout inputLayout)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetInputLayout(inputLayout);
}

void DX12_SetBlendState(BlendState blendState)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetBlendState(blendState);
}

void DX12_SetDepthState(DepthStencilState depthState)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetDepthState(depthState);
}

void DX12_SetRasterizerState(RasterizerState rasterizerState)
{
    ScopedStateManager SM = g_renderer->GetStateManager(false);
    SM->SetRasterizerState(rasterizerState);
}

void DX12_ClearRTVandDSV()
{
    g_renderEnv->ClearScreen();
}

void DX12_Present()
{
    g_renderEnv->Present();
}