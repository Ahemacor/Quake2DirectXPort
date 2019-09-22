#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <Windows.h>
#include <dxgi.h>

#ifdef __cplusplus
extern "C" {
#endif
    void DX12_Init();
    void DX12_Release();

    void DX12_Render_Begin();
    void DX12_Render_End();
    void DX12_SetAppProps(HINSTANCE hInstance, WNDPROC winproc);
    bool DX12_InitWindow(int width, int height, int mode, bool fullscreen);
    void DX12_CloseWindow();
    void DX12_InitDefaultStates();
    UINT DX12_GetModesNumber();
    DXGI_MODE_DESC DX12_GetVideoMode(UINT index);
    HWND DX12_GetOsWindowHandle();
    void DX12_SetPrimitiveTopologyTriangleList();
#ifdef __cplusplus
}
#endif