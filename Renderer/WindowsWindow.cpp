#include "WindowsWindow.h"
#include <cassert>

WindowsWindow::WindowsWindow(HINSTANCE hInstance, WNDPROC proc, RECT sizePosition, bool isFullscreen, const std::string& title)
: appInstance(hInstance)
, procedure(proc)
, rectangle(sizePosition)
, fullscreen(isFullscreen)
, title(title)
{
    RegisterWindowClass();
    Initialize();
}

WindowsWindow::~WindowsWindow()
{
    if (windowHandle != nullptr)
    {
        ShowWindow(windowHandle, SW_HIDE);
        DestroyWindow(windowHandle);
        windowHandle = NULL;
    }

    UnregisterClass(className.c_str(), appInstance);
}

void WindowsWindow::Show()
{
    ShowWindow(windowHandle, SW_SHOW);
    UpdateWindow(windowHandle);

    SetForegroundWindow(windowHandle);
    SetFocus(windowHandle);
}

HWND WindowsWindow::GetHandle()
{
    return windowHandle;
}

bool WindowsWindow::Initialize()
{
    int exstyle = 0;
    int stylebits = 0;
    if (fullscreen)
    {
        exstyle = WS_EX_TOPMOST;
        stylebits = WS_POPUP | WS_VISIBLE;
    }
    else
    {
        exstyle = 0;
        stylebits = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE;
    }

    AdjustWindowRect(&rectangle, stylebits, FALSE);

    windowHandle = CreateWindowEx(
        exstyle,
        className.c_str(),
        title.c_str(),
        stylebits,
        rectangle.left, rectangle.top, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,//x, y, w, h,
        NULL,
        NULL,
        appInstance,
        NULL);

    assert(windowHandle && "Initialize() failed");

    return windowHandle != nullptr;
}

bool WindowsWindow::RegisterWindowClass()
{
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
    windowClass.lpszClassName = className.c_str();

    static HRESULT hr = ::RegisterClassExA(&windowClass);
    assert(SUCCEEDED(hr) && "RegisterWindowClass() failed");

    return SUCCEEDED(hr);
}