#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include "r_local.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>

#ifdef __cplusplus
extern "C" {
#endif
    void DX12_Init();
    void DX12_Release();

    void DX12_Render_Begin();
    void DX12_Render_End();
    void DX12_SetAppProps(HINSTANCE hInstance, WNDPROC winproc);
    int DX12_InitWindow(int width, int height, int mode, int isFullscreen);
    void DX12_CloseWindow();
    void DX12_InitDefaultStates();
    UINT DX12_GetModesNumber();
    DXGI_MODE_DESC DX12_GetVideoMode(UINT index);
    HWND DX12_GetOsWindowHandle();
    void DX12_SetPrimitiveTopologyTriangleList();

    int DX12_CreateConstantBuffer(const void* pSrcData, int bufferSize);
    void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize);
    void DX12_BindConstantBuffer(int resourceId, int slot);

    void DX12_BindVertexBuffer(UINT Slot, int resourceId);
    void DX12_SetViewport(const D3D12_VIEWPORT* pViewport);
    void DX12_Draw(UINT numOfVertices, UINT firstVertexToDraw);
    void DX12_DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation);
    const State* Dx12_GetRenderState();

    void DX12_SetVertexShader(ShaderType shaderType);
    void DX12_SetPixelShader(ShaderType shaderType);
    void DX12_SetInputLayout(InputLayout inputLayout);
    void DX12_SetBlendState(BlendState blendState);
    void DX12_SetDepthState(DepthStencilState depthState);
    void DX12_SetRasterizerState(RasterizerState rasterizerState);

#ifdef __cplusplus
}
#endif