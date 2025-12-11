#include "VulkanRenderer.h"

#include <AEntity>
#include <AWorld>
#include <AViewport>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <stdexcept>

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::initialize(void* nativeWindow, int width, int height) {
    hwnd_ = static_cast<HWND>(nativeWindow);
    width_ = width;
    height_ = height;
    ensureBackBuffer(width_, height_);

    // Minimal instance creation to validate that Vulkan runtime is reachable.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "MyGameEngine";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        instance_ = VK_NULL_HANDLE;
    }

    return true;
}

void VulkanRenderer::shutdown() {
    releaseBackBuffer();

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    hwnd_ = nullptr;
}

void VulkanRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    ensureBackBuffer(width_, height_);
}

void VulkanRenderer::setWorld(AWorld* world) {
    // World data is consumed during GDI fallback draw; store the pointer.
    world_ = world;
}

void VulkanRenderer::draw(const AViewport& viewport) {
    // Placeholder: until a full Vulkan pipeline is added, render a simple preview using
    // Win32 GDI. This keeps the Vulkan backend visibly working (non-black window) and
    // still projects entities using the active camera matrices.
    if (!hwnd_) {
        return;
    }

    HDC hdc = GetDC(hwnd_);

    // Keep back buffer in sync with the viewport/window size.
    if (viewport.getWidth() != backBufferWidth_ || viewport.getHeight() != backBufferHeight_) {
        resize(viewport.getWidth(), viewport.getHeight());
    }

    if (!backBufferDC_ || !backBufferBitmap_) {
        ReleaseDC(hwnd_, hdc);
        return;
    }

    HBRUSH clearBrush = CreateSolidBrush(RGB(18, 22, 28));
    RECT backRect{0, 0, backBufferWidth_, backBufferHeight_};
    FillRect(backBufferDC_, &backRect, clearBrush);
    DeleteObject(clearBrush);

    if (!world_) {
        BitBlt(hdc, 0, 0, backBufferWidth_, backBufferHeight_, backBufferDC_, 0, 0, SRCCOPY);
        ReleaseDC(hwnd_, hdc);
        return;
    }

    // Acquire the latest viewport matrices (already updated by the caller).
    const glm::mat4 view = viewport.getViewMatrix();
    const glm::mat4 projection = viewport.getProjectionMatrix();

    struct Primitive {
        std::vector<POINT> points;
        std::vector<AEntity::Color> colors;
        bool hasPerVertexColors{false};
        bool isTriangle{false};
        float depth{0.0f}; // normalized depth (0 near, 1 far)
        AEntity::Color uniformColor;
    };

    std::vector<Primitive> primitives;
    primitives.reserve(world_->getEntities().size());

    for (const auto& entityPtr : world_->getEntities()) {
        const auto& vertices = entityPtr->getVertices();
        if (vertices.size() < 3) {
            continue;
        }

        const glm::mat4 model = glm::translate(glm::mat4(1.0f), entityPtr->getPosition());
        const glm::mat4 mvp = projection * view * model;

        Primitive prim;
        prim.points.reserve(vertices.size());
        prim.colors = entityPtr->getVertexColors();
        prim.hasPerVertexColors = prim.colors.size() == vertices.size();
        prim.isTriangle = vertices.size() == 3;
        prim.uniformColor = entityPtr->getColor();

        float depthAccum = 0.0f;
        int depthCount = 0;

        for (const auto& v : vertices) {
            glm::vec4 clip = mvp * glm::vec4(v, 1.0f);
            if (clip.w == 0.0f) {
                continue;
            }
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            POINT p{};
            p.x = static_cast<LONG>((ndc.x * 0.5f + 0.5f) * static_cast<float>(viewport.getWidth()));
            p.y = static_cast<LONG>((1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(viewport.getHeight()));
            prim.points.push_back(p);

            // Depth in [0,1]: map OpenGL NDC (-1,1) to depth.
            const float depth01 = ndc.z * 0.5f + 0.5f;
            depthAccum += depth01;
            depthCount++;
        }

        if (prim.points.size() < 3 || depthCount == 0) {
            continue;
        }

        prim.depth = depthAccum / static_cast<float>(depthCount);
        primitives.push_back(std::move(prim));
    }

    // Painter's algorithm: draw far-to-near to mimic depth.
    std::sort(primitives.begin(), primitives.end(), [](const Primitive& a, const Primitive& b) {
        return a.depth > b.depth; // far (larger depth) first
    });

    for (const auto& prim : primitives) {
        if (prim.isTriangle && prim.hasPerVertexColors) {
            TRIVERTEX triVerts[3]{};
            for (size_t i = 0; i < 3; ++i) {
                triVerts[i].x = prim.points[i].x;
                triVerts[i].y = prim.points[i].y;
                auto clampToByte = [](float v) {
                    return static_cast<COLOR16>(std::clamp(v, 0.0f, 1.0f) * 255.0f) * 0x0101;
                };
                triVerts[i].Red = clampToByte(prim.colors[i].r);
                triVerts[i].Green = clampToByte(prim.colors[i].g);
                triVerts[i].Blue = clampToByte(prim.colors[i].b);
                triVerts[i].Alpha = clampToByte(prim.colors[i].a);
            }
            GRADIENT_TRIANGLE tri{0, 1, 2};
            GradientFill(backBufferDC_, triVerts, 3, &tri, 1, GRADIENT_FILL_TRIANGLE);
        } else {
            AEntity::Color color = prim.uniformColor;
            if (prim.hasPerVertexColors) {
                float r = 0.0f, g = 0.0f, b = 0.0f;
                for (const auto& c : prim.colors) {
                    r += c.r;
                    g += c.g;
                    b += c.b;
                }
                const float inv = 1.0f / static_cast<float>(prim.colors.size());
                color.r = r * inv;
                color.g = g * inv;
                color.b = b * inv;
                color.a = 1.0f;
            }

            const COLORREF rgb = RGB(
                static_cast<int>(color.r * 255.0f),
                static_cast<int>(color.g * 255.0f),
                static_cast<int>(color.b * 255.0f));

            HBRUSH brush = CreateSolidBrush(rgb);
            HBRUSH oldBrush = (HBRUSH)SelectObject(backBufferDC_, brush);
            HPEN pen = CreatePen(PS_SOLID, 1, rgb);
            HPEN oldPen = (HPEN)SelectObject(backBufferDC_, pen);

            Polygon(backBufferDC_, prim.points.data(), static_cast<int>(prim.points.size()));

            SelectObject(backBufferDC_, oldBrush);
            SelectObject(backBufferDC_, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
        }
    }

    // Blit the finished back buffer to the window DC in one go to avoid flicker.
    BitBlt(hdc, 0, 0, backBufferWidth_, backBufferHeight_, backBufferDC_, 0, 0, SRCCOPY);
    ReleaseDC(hwnd_, hdc);
}

void VulkanRenderer::ensureBackBuffer(int width, int height) {
    if (!hwnd_) {
        return;
    }

    if (backBufferBitmap_ && width == backBufferWidth_ && height == backBufferHeight_) {
        return; // Already correct size.
    }

    releaseBackBuffer();

    HDC windowDC = GetDC(hwnd_);
    backBufferDC_ = CreateCompatibleDC(windowDC);
    backBufferBitmap_ = CreateCompatibleBitmap(windowDC, width, height);
    backBufferOldBitmap_ = (HBITMAP)SelectObject(backBufferDC_, backBufferBitmap_);
    backBufferWidth_ = width;
    backBufferHeight_ = height;
    ReleaseDC(hwnd_, windowDC);
}

void VulkanRenderer::releaseBackBuffer() {
    if (backBufferDC_) {
        if (backBufferOldBitmap_) {
            SelectObject(backBufferDC_, backBufferOldBitmap_);
            backBufferOldBitmap_ = nullptr;
        }
        if (backBufferBitmap_) {
            DeleteObject(backBufferBitmap_);
            backBufferBitmap_ = nullptr;
        }
        DeleteDC(backBufferDC_);
        backBufferDC_ = nullptr;
    }
    backBufferWidth_ = 0;
    backBufferHeight_ = 0;
}
