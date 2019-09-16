#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif
    void Render_Begin();
    void Render_End();
    void StartApp(HINSTANCE hInstance, WNDPROC winproc);
#ifdef __cplusplus
}
#endif