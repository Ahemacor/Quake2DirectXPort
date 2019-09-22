#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif
#include <d3d11.h>
#include "r_local.h"
#ifdef __cplusplus
extern "C" {
#endif
// Common interface.
void CPPInit();
void CPPRease();

// Render Window interface.
UINT RWGetModesNumber();
DXGI_MODE_DESC RWGetMode(UINT index);

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

ID3D11RenderTargetView* RWGetRTV();
ID3D11RenderTargetView** RWGetRTVAddr();
ID3D11DepthStencilView* RWGetDSV();

void RWSetPrimitiveTopologyTriangleList();

void RWRenderBegin();
void RWRenderEnd();

// State Manager interface.
void SMSetRenderStates(BlendState bs, DepthStencilState ds, RasterizerState rs);
void SMInitDefaultStates();
void SMBindVertexBuffer(UINT Slot, ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
void SMBindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format);
void SMBindSamplers();

// Shader Loader interface.
void SLInitShaders();
void SLShutdownShaders();
void SLRegisterConstantBuffer(ID3D11Buffer* cBuffer, int slot);
int SLCreateShaderBundle(int resourceID, const char* vsentry, const char* gsentry, const char* psentry, D3D11_INPUT_ELEMENT_DESC* layout, int numlayout);
void SLBindShaderBundle(int sb);
void SLBindConstantBuffers();

#ifdef __cplusplus
}
#endif