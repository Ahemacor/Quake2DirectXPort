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

    void SetDevice(ID3D11Device* pDevice);
    ID3D11Device* GetDevice();

    void SetDeviceContext(ID3D11DeviceContext* pDeviceContext);
    ID3D11DeviceContext* GetDeviceContext();

    void SetSwapchain(IDXGISwapChain* pSwapchain);
    IDXGISwapChain* GetSwapchain();

    void SetRTV(ID3D11RenderTargetView* pRTV);
    ID3D11RenderTargetView* GetRTV();
    ID3D11RenderTargetView** GetRTVAddr();

    void SetDSV(ID3D11DepthStencilView* pDSV);
    ID3D11DepthStencilView* GetDSV();

    HWND GetWindowHandle();

    void SetAppProps(HINSTANCE hInstance_, WNDPROC wndproc_)
    {
        hInstance = hInstance_;
        wndproc = wndproc_;
    }

    bool InitDirectX();
    bool InitWindow(int width, int height, bool fullscreen);
    void CloseWindow();

    void SetMode(int mode) { currentMode = mode; }

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
