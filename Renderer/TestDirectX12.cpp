#include "TestDirectX12.h"
#include "WindowsWindow.h"
#include "RenderEnvironment.h"
#include "Renderer.h"

WindowsWindow* g_window = nullptr;
RenderEnvironment* g_renderEnv = nullptr;
Renderer* g_renderer = nullptr;

uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

bool g_VSync = true;
bool g_TearingSupported = false;
bool g_Fullscreen = false;

const char window_class[] = "DirectX12Class";
const char window_title[] = "Test DirectX 12 Window";

void DX12_Init()
{

}

void DX12_Release()
{

}

void DX12_Render_Begin()
{
    g_renderEnv->ClearScreen();
    g_renderer->RenderImpl();
}

void DX12_Render_End()
{
    g_renderEnv->Present();
}

void DX12_StartApp(HINSTANCE hInstance, WNDPROC winproc)
{
    RECT winrect = {};
    winrect.left = 0;
    winrect.top = 0;
    winrect.right = g_ClientWidth;
    winrect.bottom = g_ClientHeight;
    g_window = new WindowsWindow(hInstance, winproc, winrect, g_Fullscreen, window_title);
    g_renderEnv = new RenderEnvironment(g_window->GetHandle(), g_ClientWidth, g_ClientHeight);
    g_renderer = new Renderer();
    g_renderEnv->InitializeAll();
    g_renderer->Init(g_renderEnv);
    g_window->Show();
}