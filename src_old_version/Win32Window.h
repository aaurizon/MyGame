#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

struct WindowSize
{
    uint32_t width = 0;
    uint32_t height = 0;
};

class Win32Window
{
public:
    Win32Window(HINSTANCE instance, const char* title, int width, int height);
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    HWND GetHwnd() const { return hwnd_; }
    HINSTANCE GetInstance() const { return hInstance_; }
    bool IsValid() const { return hwnd_ != nullptr; }
    WindowSize GetClientSize() const;
    // Returns true if a resize occurred since last check and resets the flag.
    bool ConsumeResizeFlag();

    // Pumps all pending messages; returns false when WM_QUIT is received or an error occurs.
    bool PumpMessages(MSG& msg);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterWindowClass();
    void UnregisterWindowClass();

    HINSTANCE hInstance_{};
    HWND hwnd_{};
    const char* className_ = "VulkanWin32WindowClass";
    bool framebufferResized_ = false;
};
