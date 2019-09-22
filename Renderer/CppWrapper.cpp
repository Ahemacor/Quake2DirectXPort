#include "CppWrapper.h"
#include <cassert>
#include <string>

#if DX11_IMPL
#include "RenderWindow.h"
#include "StateManager.h"
#include "ShaderLoader.h"

RenderWindow* g_pRenderWindow = nullptr;
StateManager* g_pStateManager = nullptr;
ShaderLoader* g_pShaderLoader = nullptr;

#else //!DX11_IMPL
#include "TestDirectX12.h"
#endif // !DX11_IMPL

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

// Common interface.

void CPPInit()
{
#if DX11_IMPL
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

    if (g_pShaderLoader != nullptr)
    {
        delete g_pShaderLoader;
    }
    g_pShaderLoader = new ShaderLoader();
    assert(g_pShaderLoader != nullptr);
#else // !DX11_IMPL
    DX12_Init();
#endif // DX11_IMPL
}

void CPPRease()
{
#if DX11_IMPL
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
#else // !DX11_IMPL
    DX12_Release();
#endif // DX11_IMPL
}

// Render Window interface.

void RWSetAppProps(HINSTANCE hInstance, WNDPROC wndproc)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->SetAppProps(hInstance, wndproc);
#else
    DX12_SetAppProps(hInstance, wndproc);
#endif // DX11_IMPL
}

qboolean RWInitWindow(int width, int height, int mode, qboolean fullscreen)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->InitWindow(width, height, mode, fullscreen);
#else // !DX11_IMPL
    return DX12_InitWindow(width, height, mode, fullscreen);
#endif // DX11_IMPL
}

void RWClose()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CloseWindow();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void* RWGetHandle()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return (void*)g_pRenderWindow->GetWindowHandle();
#else // !DX11_IMPL
    return DX12_GetOsWindowHandle();
#endif // DX11_IMPL
}

ID3D11Device* RWGetDevice()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDevice();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

void RWCreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->CreateBuffer(pDesc, pSrcMem, outBufferAddr);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

HRESULT RWCreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return 0;
#endif // DX11_IMPL
}

HRESULT RWCreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->CreateShaderResourceView(pResource, pDesc, ppSRView);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return 0;
#endif // DX11_IMPL
}

ID3D11DeviceContext* RWGetDeviceContext()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDeviceContext();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

IDXGISwapChain* RWGetSwapchain()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetSwapchain();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

ID3D11RenderTargetView* RWGetRTV()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTV();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

ID3D11RenderTargetView** RWGetRTVAddr()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetRTVAddr();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

ID3D11DepthStencilView* RWGetDSV()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetDSV();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return nullptr;
#endif // DX11_IMPL
}

void RWSetPrimitiveTopologyTriangleList()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    g_pRenderWindow->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void RWRenderBegin()
{
#if DX11_IMPL
    NOT_IMPL_FAIL();
#else // !DX11_IMPL
    DX12_Render_Begin();
#endif // DX11_IMPL
}

void RWRenderEnd()
{
#if DX11_IMPL
    NOT_IMPL_FAIL();
#else // !DX11_IMPL
    DX12_Render_End();
#endif // DX11_IMPL
}

UINT RWGetModesNumber()
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetModesNumber();
#else // !DX11_IMPL
    return DX12_GetModesNumber();
#endif // DX11_IMPL
}

DXGI_MODE_DESC RWGetMode(UINT index)
{
#if DX11_IMPL
    assert(g_pRenderWindow != nullptr);
    return g_pRenderWindow->GetMode(index);
#else // !DX11_IMPL
    return DX12_GetVideoMode(index);
#endif // DX11_IMPL
}

// State Manager interface.

void SMSetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs)
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetRenderStates(bs, ds, rs);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SMInitDefaultStates()
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->SetDefaultState();
#else // !DX11_IMPL
    DX12_InitDefaultStates();
#endif // DX11_IMPL
}

void SMBindVertexBuffer(UINT Slot, ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindVertexBuffer(Slot, Buffer, Stride, Offset);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SMBindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindIndexBuffer(Buffer, Format);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SMBindSamplers()
{
#if DX11_IMPL
    assert(g_pStateManager != nullptr);
    g_pStateManager->BindSamplers();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

// Shader Loader interface.

void SLInitShaders()
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->InitShaders();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SLShutdownShaders()
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->ShutdownShaders();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SLRegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot)
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->RegisterConstantBuffer(cBuffer, slot);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

int SLCreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout)
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    return g_pShaderLoader->CreateShaderBundle(resourceID, vsentry, gsentry, psentry, layout, numlayout);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
    return 0;
#endif // DX11_IMPL
}

void SLBindShaderBundle(int sb)
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->BindShaderBundle(sb);
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}

void SLBindConstantBuffers()
{
#if DX11_IMPL
    assert(g_pShaderLoader != nullptr);
    g_pShaderLoader->BindConstantBuffers();
#else // !DX11_IMPL
    NOT_IMPL_FAIL();
#endif // DX11_IMPL
}