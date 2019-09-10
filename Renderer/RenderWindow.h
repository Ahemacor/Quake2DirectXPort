#pragma once
#ifdef __cplusplus
#undef CINTERFACE

#include "d3dcommon.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

struct vidmenu_t;

class RenderWindow
{
public:
    IDXGIAdapter* GetFirstAdapter();
    IDXGIOutput* GetAdapterOutput(IDXGIAdapter* pAdapter);
    std::vector<DXGI_MODE_DESC> GetOutputVideoModes(IDXGIOutput* output, DXGI_FORMAT fmt);
    void GetSuitableVideoModes(const DXGI_MODE_DESC* outModes, UINT* outSize);
    int GetModeWidth(int mode);
    int GetModeHeight(int mode);

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


private:
    /*ID3D11Device* d3d_Device = NULL;
    ID3D11DeviceContext* d3d_Context = NULL;
    IDXGISwapChain* d3d_SwapChain = NULL;

    ID3D11RenderTargetView* d3d_RenderTarget = NULL;
    ID3D11DepthStencilView* d3d_DepthBuffer = NULL;*/

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;

    std::vector<DXGI_MODE_DESC> d3d_VideoModes;
};
#endif
