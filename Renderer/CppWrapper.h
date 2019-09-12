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

void RWSetAppProps(HINSTANCE hInstance, WNDPROC wndproc);
qboolean RWInitWindow(int width, int height, int mode, qboolean fullscreen);
void RWClose();

void* RWGetHandle();

ID3D11Device* RWGetDevice();
ID3D11DeviceContext* RWGetDeviceContext();
IDXGISwapChain* RWGetSwapchain();

void RWCreateBuffer(const D3D11_BUFFER_DESC* pDesc, const void* pSrcMem, ID3D11Buffer** outBufferAddr);
HRESULT RWCreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D);
HRESULT RWCreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView);

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