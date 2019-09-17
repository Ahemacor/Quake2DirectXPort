#pragma once

#include <Windows.h>
#include <string>

class WindowsWindow
{
public:
    WindowsWindow(HINSTANCE hInstance, WNDPROC proc, RECT sizePosition, bool isFullscreen = false , const std::string& title = "Window");
    ~WindowsWindow();
    void Show();
    HWND GetHandle();

private:
    WindowsWindow(const WindowsWindow&) = delete;
    WindowsWindow& operator=(const WindowsWindow&) = delete;

    bool Initialize();
    bool RegisterWindowClass();

    HINSTANCE appInstance = nullptr;
    WNDPROC procedure = nullptr;
    RECT rectangle;
    bool fullscreen = false;
    const std::string title;
    const std::string className = "WindowsWindow";

    HWND windowHandle = nullptr;
};
