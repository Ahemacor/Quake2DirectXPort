#include "WindowsWindow.h"
#include <cassert>

WindowsWindow::WindowsWindow() {}

WindowsWindow::~WindowsWindow()
{
    Hide();
}

void WindowsWindow::SetParentInstance(HINSTANCE inst)
{
    assert(inst != nullptr);
    appInstance = inst;
}

void WindowsWindow::SetWindowProcedure(WNDPROC proc)
{
    assert(proc != nullptr);
    procedure = proc;
}

bool WindowsWindow::Show(RECT sizePosition, bool isFullscreen, const std::string& title)
{
    bool winInitSuccess = RegisterWindowClass();

    if (winInitSuccess)
    {
        windowHandle = CreateOSWindow(sizePosition, isFullscreen, title);
        if (windowHandle != nullptr)
        {
            ShowWindow(windowHandle, SW_SHOW);
            UpdateWindow(windowHandle);

            SetForegroundWindow(windowHandle);
            SetFocus(windowHandle);
        }
        else
        {
            winInitSuccess = false;
        }
    }

    return winInitSuccess;
}

void WindowsWindow::Hide()
{
    if (windowHandle != nullptr)
    {
        ShowWindow(windowHandle, SW_HIDE);
        DestroyWindow(windowHandle);
        windowHandle = nullptr;
    }

    UnregisterClass(CLASS_NAME.c_str(), appInstance);
}

HWND WindowsWindow::GetHandle()
{
    return windowHandle;
}

HWND WindowsWindow::CreateOSWindow(RECT rect, bool isFullscreen, const std::string& title)
{
    assert(appInstance != nullptr);

    int exstyle = 0;
    int stylebits = 0;
    if (isFullscreen)
    {
        exstyle = WS_EX_TOPMOST;
        stylebits = WS_POPUP | WS_VISIBLE;
    }
    else
    {
        exstyle = 0;
        stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE;
    }

    AdjustWindowRect(&rect, stylebits, FALSE);

    HWND handle = CreateWindowEx(exstyle,
                                 CLASS_NAME.c_str(),
                                 title.c_str(),
                                 stylebits,
                                 rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, //x, y, w, h,
                                 NULL,
                                 NULL,
                                 appInstance,
                                 NULL);
    assert(handle != nullptr);

    return handle;
}

bool WindowsWindow::RegisterWindowClass()
{
    assert(procedure != nullptr);
    assert(appInstance != nullptr);

    // Register a window class for creating our render window with.
    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = 0;
    windowClass.lpfnWndProc = procedure;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = appInstance;
    windowClass.hIcon = 0;
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = CLASS_NAME.c_str();

    static HRESULT hr = ::RegisterClassExA(&windowClass);
    assert(SUCCEEDED(hr) && "RegisterWindowClass() failed");

    return SUCCEEDED(hr);
}