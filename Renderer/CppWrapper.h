#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif
#include <d3d11.h>
#ifdef __cplusplus
extern "C" {
#endif

void Init();
void Release();

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

UINT GetModesNumber();

DXGI_MODE_DESC GetMode(UINT index);

#ifdef __cplusplus
}
#endif