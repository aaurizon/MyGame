#include "../src/Win32Window.h"

#include <iostream>

WindowSize Win32Window::GetClientSize() const
{
    RECT rect{};
    if (hwnd_ && GetClientRect(hwnd_, &rect))
    {
        return {static_cast<uint32_t>(rect.right - rect.left), static_cast<uint32_t>(rect.bottom - rect.top)};
    }
    return {};
}

bool Win32Window::ConsumeResizeFlag()
{
    bool wasResized = framebufferResized_;
    framebufferResized_ = false;
    return wasResized;
}

Win32Window::Win32Window(HINSTANCE instance, const char* title, int width, int height)
    : hInstance_(instance)
{
    RegisterWindowClass();

    RECT rect{0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindowExA(
        0,
        className_,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance_,
        this);

    if (!hwnd_)
    {
        MessageBoxA(nullptr, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        UnregisterWindowClass();
        return;
    }

    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);
}

Win32Window::~Win32Window()
{
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    UnregisterWindowClass();
}

bool Win32Window::PumpMessages(MSG& msg)
{
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

void Win32Window::RegisterWindowClass()
{
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Win32Window::WindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance_;
    wc.hIcon         = LoadIconA(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = className_;
    wc.hIconSm       = LoadIconA(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc))
    {
        MessageBoxA(nullptr, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
    }
}

void Win32Window::UnregisterWindowClass()
{
    if (hInstance_)
    {
        ::UnregisterClassA(className_, hInstance_);
    }
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCTA* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        auto* window = reinterpret_cast<Win32Window*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
    }

    auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (window)
        {
            window->framebufferResized_ = true;
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
