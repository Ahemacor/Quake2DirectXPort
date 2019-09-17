#include "CppWrapper.h"
#include "RenderWindow.h"
#include "StateManager.h"
#include "ShaderLoader.h"
#include <cassert>

#if !DX11_IMPL
#include "TestDirectX12.h"
#endif // !DX11_IMPL

RenderWindow* g_pRenderWindow = nullptr;
StateManager* g_pStateManager = nullptr;
ShaderLoader* g_pShaderLoader = nullptr;

// Common interface.

void CPPInit()
{
    /*if (g_pRenderWindow != nullptr)
    {
        delete g_pRenderWindow;
    }
    g_pRenderWindow = new RenderWindow();
    assert(g_pRenderWindow != nullptr);*/

    if (g_pStateManager != nullptr)
    {
        delete g_pStateManager;
    }
    g_pStateManager = new StateManager();
    assert(g_pStateManager != nullptr);

    if (g_pShaderLoader != nullptr)
    {
        delete g_pShaderLoader;
    }
    g_pShaderLoader = new ShaderLoader();
    assert(g_pShaderLoader != nullptr);
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

    if (g_pShaderLoader != nullptr)
    {
        delete g_pShaderLoader;
        g_pShaderLoader = nullptr;
    }
    assert(g_pStateManager == nullptr);
}

// Render Window interface.
HINSTANCE g_Instance = nullptr; 
WNDPROC g_wndproc = nullptr;
void RWSetAppProps(HINSTANCE hInstance, WNDPROC wndproc)
{
    g_Instance = hInstance;
    g_wndproc = wndproc;
/*#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetAppProps(hInstance, wndproc);
#else
    StartApp(hInstance, wndproc);
#endif // DX11_IMPL*/
}

qboolean RWInitWindow(int width, int height, int mode, qboolean fullscreen)
{
    g_pRenderWindow = new RenderWindow(g_Instance, g_wndproc, width, height);
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->InitWindow(width, height, mode, fullscreen);*/
}

void RWClose()
{
    /*assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CloseWindow();*/
}

void* RWGetHandle()
{
    /*assert(g_pRenderWindow != nullptr);
    return (void*)g_pRenderWindow->GetWindowHandle();*/
    return nullptr;
}

ID3D11Device* RWGetDevice()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDevice();*/
    return nullptr;
}

void RWCreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr)
{
    /*assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CreateBuffer(pDesc, pSrcMem, outBufferAddr);*/
}

HRESULT RWCreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateTexture2D(pDesc, pInitialData, ppTexture2D);*/
    return 0;
}

HRESULT RWCreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateShaderResourceView(pResource, pDesc, ppSRView);*/
}

ID3D11DeviceContext* RWGetDeviceContext()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDeviceContext();*/
    return nullptr;
}

IDXGISwapChain* RWGetSwapchain()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetSwapchain();*/
    return nullptr;
}

ID3D11RenderTargetView* RWGetRTV()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTV();*/
    return nullptr;
}

ID3D11RenderTargetView** RWGetRTVAddr()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTVAddr();*/
    return nullptr;
}

ID3D11DepthStencilView* RWGetDSV()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDSV();*/
    return nullptr;
}


UINT RWGetModesNumber()
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetModesNumber();*/
    return 0;
}

DXGI_MODE_DESC RWGetMode(UINT index)
{
    /*assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetMode(index);*/
    return {};
}

void RWClearScreen()
{
    g_pRenderWindow->dx12environment.ClearScreen();
}

void RWPresent()
{
    g_pRenderWindow->dx12environment.Present();
}

// State Manager interface.

void SMSetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs)
{
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetRenderStates(bs, ds, rs);
}

void SMInitDefaultStates()
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetDefaultState();
#endif // DX11_IMPL
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

// Shader Loader interface.

void SLInitShaders()
{
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->InitShaders();
}

void SLShutdownShaders()
{
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->ShutdownShaders();
}

void SLRegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot)
{
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->RegisterConstantBuffer(cBuffer, slot);
}

int SLCreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout)
{
    assert(g_pShaderLoader != nullptr);
    return g_pShaderLoader->CreateShaderBundle(resourceID, vsentry, gsentry, psentry, layout, numlayout);
}

void SLBindShaderBundle(int sb)
{
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->BindShaderBundle(sb);
}

void SLBindConstantBuffers()
{
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->BindConstantBuffers();
}