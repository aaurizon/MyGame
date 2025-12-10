#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

// Window title
const char* WINDOW_TITLE = "MyGame - v1.0.0";

// Forward declaration of the window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// WinMain: entry point for Win32 GUI applications
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // 1. Define and register the window class
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;      // Redraw on horizontal/vertical resize
    wc.lpfnWndProc   = WindowProc;                  // Window procedure callback
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIconA(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // Basic white background
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = "VulkanWin32WindowClass";
    wc.hIconSm       = LoadIconA(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc))
    {
        MessageBoxA(nullptr, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 2. Create the window
    const int windowWidth  = 1280;
    const int windowHeight = 720;

    // Desired client area; we adjust the rect so that the overall window has this client size
    RECT rect{ 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExA(
        0,                              // Optional extended styles
        wc.lpszClassName,              // Window class name
        WINDOW_TITLE,                  // Window title
        WS_OVERLAPPEDWINDOW,           // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,  // Position (x, y)
        rect.right - rect.left,        // Width
        rect.bottom - rect.top,        // Height
        nullptr,                       // Parent window
        nullptr,                       // Menu
        hInstance,                     // Instance handle
        nullptr                        // Additional application data
    );

    if (!hwnd)
    {
        MessageBoxA(nullptr, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Show and update the window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 3. Main message loop
    MSG msg{};
    while (true)
    {
        // GetMessage blocks until a message is available or WM_QUIT is posted
        BOOL result = GetMessageA(&msg, nullptr, 0, 0);

        if (result > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        else if (result == 0)
        {
            // WM_QUIT received: exit the loop
            break;
        }
        else
        {
            // Error
            return -1;
        }
    }

    return static_cast<int>(msg.wParam);
}

// 4. Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        // Post a WM_QUIT message to end the message loop
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        // Basic paint handling (we do nothing special here yet)
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        // You could draw text here if you want, but we leave it empty.
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        break;
    }

    // Default handling for messages we do not process
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char* argv[])
{
    // Get the HINSTANCE for this module (same as the one passed to WinMain normally)
    HINSTANCE hInstance = GetModuleHandleA(nullptr);

    // Get the command line as a single string (equivalent of lpCmdLine)
    LPSTR lpCmdLine = GetCommandLineA();

    // Choose how the window should be shown
    int nCmdShow = SW_SHOWDEFAULT;

    // hPrevInstance is always nullptr on modern Windows
    HINSTANCE hPrevInstance = nullptr;

    // Call WinMain and return its result
    return WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
