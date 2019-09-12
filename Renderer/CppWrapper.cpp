#include "CppWrapper.h"
#include "RenderWindow.h"
#include "StateManager.h"
#include <cassert>

RenderWindow* g_pRenderWindow = nullptr;
StateManager* g_pStateManager = nullptr;

void CPPInit()
{
    if (g_pRenderWindow != nullptr)
    {
        delete g_pRenderWindow;
    }
    g_pRenderWindow = new RenderWindow();
    assert(g_pRenderWindow != nullptr);

    if (g_pStateManager != nullptr)
    {
        delete g_pStateManager;
    }
    g_pStateManager = new StateManager();
    assert(g_pStateManager != nullptr);
}

void CPPRease()
{
    if (g_pRenderWindow != nullptr)
    {
        delete g_pRenderWindow;
        g_pRenderWindow = nullptr;
    }
    assert(g_pRenderWindow == nullptr);

    if (g_pStateManager != nullptr)
    {
        delete g_pStateManager;
        g_pStateManager = nullptr;
    }
    assert(g_pStateManager == nullptr);
}

void RWSetAppProps(HINSTANCE hInstance, WNDPROC wndproc)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetAppProps(hInstance, wndproc);
}

qboolean RWInitWindow(int width, int height, int mode, qboolean fullscreen)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->InitWindow(width, height, mode, fullscreen);
}

void RWClose()
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CloseWindow();
}

void* RWGetHandle()
{
    assert(g_pRenderWindow != nullptr);
    return (void*)g_pRenderWindow->GetWindowHandle();
}

ID3D11Device* RWGetDevice()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDevice();
}

void RWCreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CreateBuffer(pDesc, pSrcMem, outBufferAddr);
}

HRESULT RWCreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}

HRESULT RWCreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateShaderResourceView(pResource, pDesc, ppSRView);
}

ID3D11DeviceContext* RWGetDeviceContext()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDeviceContext();
}

IDXGISwapChain* RWGetSwapchain()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetSwapchain();
}

ID3D11RenderTargetView* RWGetRTV()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTV();
}

ID3D11RenderTargetView** RWGetRTVAddr()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTVAddr();
}

ID3D11DepthStencilView* RWGetDSV()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDSV();
}


UINT RWGetModesNumber()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetModesNumber();
}

DXGI_MODE_DESC RWGetMode(UINT index)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetMode(index);
}

void SMSetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs)
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetRenderStates(bs, ds, rs);
}

void SMInitDefaultStates()
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetDefaultState();
}

void SMBindVertexBuffer(UINT Slot, ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindVertexBuffer(Slot, Buffer, Stride, Offset);
}

void SMBindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindIndexBuffer(Buffer, Format);
}

void SMBindSamplers()
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindSamplers();
}