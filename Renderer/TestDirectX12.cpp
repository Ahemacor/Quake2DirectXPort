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
    /*int width = 0, height = 0;
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
    g_renderer->BindIndexBuffer(ibId);*/
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

    // load all of our initial objects
    // each subsystem creates it's objects then registers it's handlers, following which the reset handler runs to complete object creation
    // SLInitShaders(); // must be done first before any other shaders are created
    R_InitMain();
    R_InitSurfaces();
    R_InitParticles();
    R_InitSprites();
    R_InitLight();
    R_InitWarp();
    R_InitSky();
    R_InitMesh();
    R_InitBeam();
    R_InitNull();

    return initSuccess;
}

void DX12_CloseWindow()
{
    g_renderer->Release();
    g_renderEnv->Release();
    g_window->Hide();

    // handle special objects
    //SLShutdownShaders();
    R_ShutdownSurfaces();
    R_ShutdownLight();
    R_ShutdownWarp();
    R_ShutdownSky();
    R_ShutdownMesh();
    R_ShutdownBeam();
    R_ShutdownSprites();
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

int DX12_CreateConstantBuffer(const void* pSrcData, int bufferSize)
{
    return g_renderer->CreateConstantBuffer(bufferSize, pSrcData);
}

void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize)
{
    g_renderer->UpdateConstantBuffer(resourceId, pSrcData, bufferSize);
}

void DX12_BindConstantBuffer(int resourceId, int slot)
{
    g_renderer->BindConstantBuffer(resourceId, slot);
}

int DX12_CreateVertexBuffer(int numOfVertices, int vertexSize, const void* pVertexData)
{
    return g_renderer->CreateVertexBuffer(numOfVertices, vertexSize, pVertexData);
}

void DX12_UpdateVertexBuffer(int resourceId, const void* pVertexData, int numOfVertices, int vertexSize)
{
    g_renderer->UpdateVertexBuffer(resourceId, pVertexData, numOfVertices, vertexSize);
}

void DX12_BindVertexBuffer(UINT Slot, int resourceId, UINT Offset)
{
    g_renderer->BindVertexBuffer(Slot, resourceId, Offset);
}

int DX12_CreateIndexBuffer(int numOfIndices, const void* pIndexData, int indexSize)
{
    return g_renderer->CreateIndexBuffer(numOfIndices, pIndexData, indexSize);
}

void DX12_UpdateIndexBuffer(int resourceId, const void* pIndexData, int numOfIndices, int indexSize)
{
    g_renderer->UpdateIndexBuffer(resourceId, pIndexData, numOfIndices, indexSize);
}

void DX12_BindIndexBuffer(int resourceId)
{
    g_renderer->BindIndexBuffer(resourceId);
}

int DX12_CreateTexture(const D3D12_RESOURCE_DESC* descr, D3D12_SUBRESOURCE_DATA* pSrcData)
{
    const CD3DX12_RESOURCE_DESC Descr(*descr);
    return g_renderer->CreateTextureResource(Descr, pSrcData);
}

void DX12_UpdateTexture(int resourceId, D3D12_SUBRESOURCE_DATA* pSrcData)
{
    g_renderer->UpdateTextureResource(resourceId, pSrcData);
}

void DX12_BindTexture(UINT slot, int resourceId)
{
    g_renderer->BindTextureResource(resourceId, slot);
}

int DX12_CreateTextureBuffer(D3D12_RESOURCE_STATES resState, int numOfElements, int elementSize, const void* pSrcData)
{
    return g_renderer->CreateTextureBuffer(resState, numOfElements, elementSize, pSrcData);
}

void DX12_UpdateTextureBuffer(D3D12_RESOURCE_STATES resState, int resourceId, const void* pSrcData, int numOfElements, int elementSize)
{
    g_renderer->UpdateTextureBuffer(resState, resourceId, pSrcData, numOfElements, elementSize);
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

void DX12_SetRenderState(UINT stateId)
{
    g_renderer->SetPSO(stateId);
}

UINT DX12_CreateRenderState(const State* state)
{
    return g_renderer->CreatePSO(state);
}

void DX12_UpdateRenderState(const State* state, int stateId)
{
    g_renderer->CreatePSO(state, stateId);
}

State DX12_GetRenderState(int stateId)
{
    return g_renderer->GetStateManager()->GetStateDescr(stateId);
}

State DX12_GetCurrentRenderState()
{
    return g_renderer->GetCurrentRenderState();
}

UINT DX12_GetCurrentRenderStateId()
{
    return g_renderer->GetPSOiD();
}

void DX12_ClearRTVandDSV()
{
    g_renderEnv->ClearScreen();
}

void DX12_Present()
{
    g_renderEnv->Present();
}