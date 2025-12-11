#pragma once

#include <Graphics/IRendererImpl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <vulkan/vulkan.h>

// Vulkan renderer stub that keeps API parity with OpenGL renderer. It can be extended with
// a full Vulkan pipeline later; for now it uses a software rasterizer with a depth buffer
// to provide consistent visibility ordering while still exercising the camera matrices.
class VulkanRenderer : public IRendererImpl {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

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
    VkInstance instance_{VK_NULL_HANDLE};

    // GDI surface paired with a software depth buffer.
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
