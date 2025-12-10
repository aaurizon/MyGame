#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

    // Returns false when WM_QUIT is received or an error occurs.
    bool PumpMessages(MSG& msg);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RegisterWindowClass();
    void UnregisterWindowClass();

    HINSTANCE hInstance_{};
    HWND hwnd_{};
    const char* className_ = "VulkanWin32WindowClass";
};
