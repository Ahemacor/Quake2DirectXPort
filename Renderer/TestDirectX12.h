#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif
    void DX12_Init();
    void DX12_Release();
    void DX12_Render_Begin();
    void DX12_Render_End();
    void DX12_StartApp(HINSTANCE hInstance, WNDPROC winproc);
#ifdef __cplusplus
}
#endif