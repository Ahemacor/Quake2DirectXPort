#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include "r_local.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include "d3dx12.h"

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

    int DX12_CreateConstantBuffer(const void* pSrcData, int bufferSize);
    void DX12_UpdateConstantBuffer(int resourceId, const void* pSrcData, int bufferSize);
    void DX12_BindConstantBuffer(int resourceId, int slot);

    int DX12_CreateVertexBuffer(int numOfVertices, int vertexSize, const void* pVertexData);
    void DX12_UpdateVertexBuffer(int resourceId, const void* pVertexData, int numOfVertices, int vertexSize);
    void DX12_BindVertexBuffer(UINT Slot, int resourceId, UINT Offset);

    int DX12_CreateIndexBuffer(int numOfIndices, const void* pIndexData, int indexSize);
    void DX12_UpdateIndexBuffer(int resourceId, const void* pIndexData, int numOfIndices, int firstIndex, int indexSize);
    void DX12_BindIndexBuffer(int resourceId);

    int DX12_CreateTexture(const D3D12_RESOURCE_DESC* descr, D3D12_SUBRESOURCE_DATA* pSrcData);
    void DX12_UpdateTexture(int resourceId, D3D12_SUBRESOURCE_DATA* pSrcData);
    void DX12_BindTexture(UINT slot, int resourceId);

    void DX12_ReleaseResource(int resourceId);

    void DX12_SetViewport(const D3D12_VIEWPORT* pViewport);
    void DX12_SetRenderState(UINT stateId);
    UINT DX12_CreateRenderState(const State* state);
    void DX12_UpdateRenderState(const State* state, int stateId);
    State DX12_GetRenderState(int stateId);
    State DX12_GetCurrentRenderState();
    UINT DX12_GetCurrentRenderStateId();

    void DX12_Draw(UINT numOfVertices, UINT firstVertexToDraw);
    void DX12_DrawIndexed(UINT indexCount, UINT firstIndex, UINT baseVertexLocation);

    void DX12_ClearRTVandDSV();
    void DX12_Present(UINT SyncInterval, UINT Flags);
#ifdef __cplusplus
}
#endif