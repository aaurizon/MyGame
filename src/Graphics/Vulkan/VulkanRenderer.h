#pragma once

#include <Graphics/IRendererImpl.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <vulkan/vulkan.h>

// Vulkan renderer stub that keeps API parity with OpenGL renderer. It can be extended with
// full Vulkan pipelines later; for now it clears the window using GDI to keep runtime behavior friendly.
class VulkanRenderer : public IRendererImpl {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

    bool initialize(void* nativeWindow, int width, int height) override;
    void shutdown() override;
    void resize(int width, int height) override;
    void draw(const AViewport& viewport) override;
    void setWorld(class AWorld* world) override;

private:
    AWorld* world_{nullptr};
    HWND hwnd_{nullptr};
    int width_{0};
    int height_{0};
    VkInstance instance_{VK_NULL_HANDLE};

    // Simple double-buffered GDI fallback to avoid flicker while Vulkan is stubbed.
    HDC backBufferDC_{nullptr};
    HBITMAP backBufferBitmap_{nullptr};
    HBITMAP backBufferOldBitmap_{nullptr};
    int backBufferWidth_{0};
    int backBufferHeight_{0};

    void ensureBackBuffer(int width, int height);
    void releaseBackBuffer();
};
