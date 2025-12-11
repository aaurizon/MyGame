#pragma once

#include <Graphics/IRendererImpl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d11.h>
#include <vector>

// DirectX 11 renderer stub: uses a software rasterizer with a depth buffer to
// mimic the camera/visibility behavior while keeping the API consistent.
class DirectX11Renderer : public IRendererImpl {
public:
    DirectX11Renderer();
    ~DirectX11Renderer() override;

    bool initialize(void* nativeWindow, int width, int height) override;
    void shutdown() override;
    void resize(int width, int height) override;
    void draw(const AViewport& viewport) override;
    void setWorld(class AWorld* world) override;

    struct FontEntry {
        int size{0};
        HFONT font{nullptr};
    };

private:
    AWorld* world_{nullptr};
    HWND hwnd_{nullptr};
    int width_{0};
    int height_{0};

    HDC backBufferDC_{nullptr};
    HBITMAP backBufferBitmap_{nullptr};
    HBITMAP backBufferOldBitmap_{nullptr};
    uint8_t* colorBits_{nullptr};
    int colorStride_{0};
    int backBufferWidth_{0};
    int backBufferHeight_{0};
    std::vector<float> depthBuffer_;
    std::vector<FontEntry> fontCache_;

    void ensureBackBuffer(int width, int height);
    void releaseBackBuffer();
};
