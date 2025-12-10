#include "Win32Window.h"
#include "VulkanApp.h"

const char* WINDOW_TITLE = "MyGame - v1.0.0";

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    const int windowWidth = 1280;
    const int windowHeight = 720;

    Win32Window window(hInstance, WINDOW_TITLE, windowWidth, windowHeight);
    if (!window.IsValid())
    {
        return -1;
    }

    VulkanApp app;
    if (!app.Initialize(WINDOW_TITLE, window))
    {
        MessageBoxA(window.GetHwnd(), "Failed to initialize Vulkan.", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    MSG msg{};
    while (window.PumpMessages(msg))
    {
        if (!app.DrawFrame(window))
        {
            MessageBoxA(window.GetHwnd(), "Render error.", "Error", MB_OK | MB_ICONERROR);
            break;
        }
    }

    return static_cast<int>(msg.wParam);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    HINSTANCE hInstance = GetModuleHandleA(nullptr);
    LPSTR lpCmdLine = GetCommandLineA();
    int nCmdShow = SW_SHOWDEFAULT;
    HINSTANCE hPrevInstance = nullptr;

    return WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
