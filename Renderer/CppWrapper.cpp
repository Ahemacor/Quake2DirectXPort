#include "CppWrapper.h"
#include "RenderWindow.h"
#include <cassert>

RenderWindow* g_pRenderWindow = nullptr;

void Init()
{
    if (g_pRenderWindow != nullptr)
    {
        delete g_pRenderWindow;
    }

    g_pRenderWindow = new RenderWindow();

    assert(g_pRenderWindow != nullptr);
}

void Release()
{
    if (g_pRenderWindow != nullptr)
    {
        delete g_pRenderWindow;
        g_pRenderWindow = nullptr;
    }
    assert(g_pRenderWindow == nullptr);
}

void SetAppProps(HINSTANCE hInstance, WNDPROC wndproc)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetAppProps(hInstance, wndproc);
}

qboolean InitWindow(int width, int height, qboolean fullscreen)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->InitWindow(width, height, fullscreen);
}

void CloseRenderWindow()
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CloseWindow();
}

void* GetWindowHandle()
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

void SetRTV(ID3D11RenderTargetView* pRTV)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetRTV(pRTV);
}

ID3D11RenderTargetView* GetRTV()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTV();
}

ID3D11RenderTargetView** GetRTVAddr()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTVAddr();
}

void SetDSV(ID3D11DepthStencilView* pDSV)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetDSV(pDSV);
}

ID3D11DepthStencilView* GetDSV()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDSV();
}


UINT GetModesNumber()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetModesNumber();
}

DXGI_MODE_DESC GetMode(UINT index)
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetMode(index);
}

void SetMode(UINT modeIndex)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetMode(modeIndex);
}