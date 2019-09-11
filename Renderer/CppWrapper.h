#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif
#include <d3d11.h>
#include "r_local.h"
#ifdef __cplusplus
extern "C" {
#endif

void Init();
void Release();

void SetAppProps(HINSTANCE hInstance, WNDPROC wndproc);
qboolean InitWindow(int width, int height, qboolean fullscreen);
void CloseRenderWindow();

void* GetWindowHandle();

ID3D11Device* RWGetDevice();
ID3D11DeviceContext* RWGetDeviceContext();
IDXGISwapChain* RWGetSwapchain();

void RWCreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr);

void SetRTV(ID3D11RenderTargetView* pRTV);
ID3D11RenderTargetView* GetRTV();
ID3D11RenderTargetView** GetRTVAddr();

void SetDSV(ID3D11DepthStencilView* pDSV);
ID3D11DepthStencilView* GetDSV();

UINT GetModesNumber();
DXGI_MODE_DESC GetMode(UINT index);
void SetMode(UINT modeIndex);

#ifdef __cplusplus
}
#endif