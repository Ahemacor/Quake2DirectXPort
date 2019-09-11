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

void SetDevice(ID3D11Device* pDevice)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetDevice(pDevice);
}

ID3D11Device* GetDevice()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDevice();
}

void SetDeviceContext(ID3D11DeviceContext* pDeviceContext)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetDeviceContext(pDeviceContext);
}

ID3D11DeviceContext* GetDeviceContext()
{
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDeviceContext();
}

void SetSwapchain(IDXGISwapChain* pSwapchain)
{
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetSwapchain(pSwapchain);
}

IDXGISwapChain* GetSwapchain()
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