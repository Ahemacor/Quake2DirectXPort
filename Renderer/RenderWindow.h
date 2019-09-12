#pragma once
#ifdef __cplusplus
#undef CINTERFACE

#include "d3dcommon.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <windows.h>
#include <vector>
#include <string>

class RenderWindow
{
public:
    RenderWindow();
    ~RenderWindow();

    IDXGIAdapter* GetFirstAdapter();
    IDXGIOutput* GetAdapterOutput(IDXGIAdapter* pAdapter);
    std::vector<DXGI_MODE_DESC> GetOutputVideoModes(IDXGIOutput* output, DXGI_FORMAT fmt);
    void LoadModes();
    UINT GetModesNumber();
    DXGI_MODE_DESC GetMode(UINT index);

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetDeviceContext();
    IDXGISwapChain* GetSwapchain();

    void CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr);
    HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D);
    HRESULT CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView);

    ID3D11RenderTargetView* GetRTV();
    ID3D11RenderTargetView** GetRTVAddr();

    ID3D11DepthStencilView* GetDSV();

    HWND GetWindowHandle();

    void SetAppProps(HINSTANCE hInstance_, WNDPROC wndproc_)
    {
        hInstance = hInstance_;
        wndproc = wndproc_;
    }

    bool InitDirectX();
    bool InitWindow(int width, int height, int mode, bool fullscreen);
    void CloseWindow();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;

    std::vector<DXGI_MODE_DESC> d3d_VideoModes;
    int currentMode = 0;

    HWND handle = nullptr;
    HINSTANCE hInstance = nullptr;
    WNDPROC wndproc = nullptr;
    std::string window_title = "Quake2 DirectX port";
    std::string window_class = "Quake2DirectxWindowClass";
    int width = 0;
    int height = 0;
    bool fullscreen = false;
};
#endif
