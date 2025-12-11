#include "AWindowImplWin32.h"

#include <AEvent>
#include <EEventKey>
#include <cassert>
#include <windowsx.h>

namespace {

constexpr const char* kMainWindowClass = "AWindowClass";

EEventKey::Scancode mapVirtualKey(WPARAM wParam) {
    switch (wParam) {
    case VK_ESCAPE: return EEventKey::Scancode::Escape;
    case 'W': return EEventKey::Scancode::W;
    case 'Z': return EEventKey::Scancode::W; // AZERTY forward
    case 'A': return EEventKey::Scancode::A;
    case 'Q': return EEventKey::Scancode::A; // AZERTY left
    case 'S': return EEventKey::Scancode::S;
    case 'D': return EEventKey::Scancode::D;
    case 'P': return EEventKey::Scancode::P;
    case 'O': return EEventKey::Scancode::O;
    case 'R': return EEventKey::Scancode::R;
    case 'F': return EEventKey::Scancode::F;
    case 'L': return EEventKey::Scancode::L;
    case VK_UP: return EEventKey::Scancode::Up;
    case VK_DOWN: return EEventKey::Scancode::Down;
    case VK_LEFT: return EEventKey::Scancode::Left;
    case VK_RIGHT: return EEventKey::Scancode::Right;
    case VK_SPACE: return EEventKey::Scancode::Space;
    case VK_SHIFT: return EEventKey::Scancode::Shift;
    case VK_CONTROL: return EEventKey::Scancode::Control;
    default: return EEventKey::Scancode::Unknown;
    }
}

EEventKey::Scancode mapMouseButton(UINT msg) {
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        return EEventKey::Scancode::MouseLeft;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        return EEventKey::Scancode::MouseRight;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        return EEventKey::Scancode::MouseMiddle;
    default:
        return EEventKey::Scancode::Unknown;
    }
}

} // namespace

AWindowImplWin32::AWindowImplWin32() = default;

AWindowImplWin32::~AWindowImplWin32() {
    close();
}

bool AWindowImplWin32::create(const std::string& title, int width, int height, void* parentHandle, int x, int y, bool child) {
    width_ = width;
    height_ = height;
    child_ = child;

    HINSTANCE instance = GetModuleHandleA(nullptr);

    static bool registered = false;
    if (!registered) {
        WNDCLASSA wc{};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = &AWindowImplWin32::WndProc;
        wc.hInstance = instance;
        wc.lpszClassName = kMainWindowClass;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassA(&wc)) {
            return false;
        }
        registered = true;
    }

    DWORD style = child ? (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DISABLED) : (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
    HWND parentHwnd = static_cast<HWND>(parentHandle);
    int posX = child ? x : CW_USEDEFAULT;
    int posY = child ? y : CW_USEDEFAULT;

    hwnd_ = CreateWindowExA(
        child ? WS_EX_NOPARENTNOTIFY : 0,
        kMainWindowClass,
        title.c_str(),
        style,
        posX, posY,
        width_, height_,
        parentHwnd,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    open_ = true;
    return true;
}

void AWindowImplWin32::close() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    open_ = false;
}

bool AWindowImplWin32::isOpen() const {
    return open_;
}

std::vector<std::shared_ptr<AEvent>> AWindowImplWin32::pollEvents() {
    events_.clear();

    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    std::vector<std::shared_ptr<AEvent>> collected;
    collected.swap(events_);
    return collected;
}

void* AWindowImplWin32::getNativeHandle() const {
    return hwnd_;
}

int AWindowImplWin32::getWidth() const {
    return width_;
}

int AWindowImplWin32::getHeight() const {
    return height_;
}

void AWindowImplWin32::setTitle(const std::string& title) {
    if (hwnd_) {
        SetWindowTextA(hwnd_, title.c_str());
    }
}

void AWindowImplWin32::setRect(int x, int y, int width, int height) {
    if (!hwnd_) {
        return;
    }
    SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    width_ = width;
    height_ = height;
}

void AWindowImplWin32::setCursorGrabbed(bool grabbed) {
    if (!hwnd_) {
        return;
    }
    if (grabbed == cursorGrabbed_) {
        return;
    }

    cursorGrabbed_ = grabbed;
    if (cursorGrabbed_) {
        ShowCursor(FALSE);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        POINT ul{rect.left, rect.top};
        POINT lr{rect.right, rect.bottom};
        MapWindowPoints(hwnd_, nullptr, &ul, 1);
        MapWindowPoints(hwnd_, nullptr, &lr, 1);
        RECT clipRect{ul.x, ul.y, lr.x, lr.y};
        ClipCursor(&clipRect);
        centerCursor();
    } else {
        ClipCursor(nullptr);
        ShowCursor(TRUE);
    }
}

bool AWindowImplWin32::isCursorGrabbed() const {
    return cursorGrabbed_;
}

LRESULT CALLBACK AWindowImplWin32::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AWindowImplWin32* self = reinterpret_cast<AWindowImplWin32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<AWindowImplWin32*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (!self) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_CLOSE: {
        self->open_ = false;
        self->pushEvent(std::make_shared<AEvent::Closed>());
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_DESTROY:
        self->open_ = false;
        if (!self->child_) {
            PostQuitMessage(0);
        }
        return 0;
    case WM_SIZE:
        self->handleSizeChange(lParam);
        break;
    case WM_KEYDOWN: {
        auto scancode = mapVirtualKey(wParam);
        if (scancode != EEventKey::Scancode::Unknown) {
            self->pushEvent(std::make_shared<AEvent::KeyPressed>(scancode));
        }
        break;
    }
    case WM_KEYUP: {
        auto scancode = mapVirtualKey(wParam);
        if (scancode != EEventKey::Scancode::Unknown) {
            self->pushEvent(std::make_shared<AEvent::KeyReleased>(scancode));
        }
        break;
    }
    case WM_MOUSEMOVE: {
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        glm::vec2 delta{x - self->lastMouse_.x, y - self->lastMouse_.y};
        self->lastMouse_ = {x, y};
        self->pushEvent(std::make_shared<AEvent::MouseMoved>(delta.x, delta.y, x, y));
        if (self->cursorGrabbed_) {
            self->centerCursor();
        }
        break;
    }
    case WM_ERASEBKGND:
        // Prevent the default background erase to avoid flicker; the renderer handles clearing.
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
        auto scancode = mapMouseButton(msg);
        self->pushEvent(std::make_shared<AEvent::MouseButtonPressed>(scancode));
        if (self->cursorGrabbed_) {
            self->centerCursor();
        }
        break;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        auto scancode = mapMouseButton(msg);
        self->pushEvent(std::make_shared<AEvent::MouseButtonReleased>(scancode));
        break;
    }
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void AWindowImplWin32::pushEvent(const std::shared_ptr<AEvent>& event) {
    events_.push_back(event);
}

void AWindowImplWin32::handleSizeChange(LPARAM lParam) {
    width_ = LOWORD(lParam);
    height_ = HIWORD(lParam);
}

void AWindowImplWin32::centerCursor() {
    if (!hwnd_ || !cursorGrabbed_) {
        return;
    }
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    POINT clientCenter{
        rect.left + (rect.right - rect.left) / 2,
        rect.top + (rect.bottom - rect.top) / 2
    };
    POINT screenCenter = clientCenter;
    ClientToScreen(hwnd_, &screenCenter);
    SetCursorPos(screenCenter.x, screenCenter.y);
    lastMouse_ = {static_cast<float>(clientCenter.x), static_cast<float>(clientCenter.y)};
}
