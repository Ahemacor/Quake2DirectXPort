#pragma once

#include <Windows.h>
#include <string>

class WindowsWindow
{
public:
    WindowsWindow();
    ~WindowsWindow();

    void SetParentInstance(HINSTANCE appInstance);
    void SetWindowProcedure(WNDPROC procedure);

    bool Show(RECT sizePosition, bool isFullscreen = false, const std::string& title = "Window");
    void Hide();

    HWND GetHandle();

private:
    WindowsWindow(const WindowsWindow&) = delete;
    WindowsWindow& operator=(const WindowsWindow&) = delete;

    const std::string CLASS_NAME = "WindowsWindow";

    HWND CreateOSWindow(RECT sizePosition, bool isFullscreen, const std::string& title);
    bool RegisterWindowClass();

    HINSTANCE appInstance = nullptr;
    WNDPROC procedure = nullptr;

    HWND windowHandle = nullptr;

};
