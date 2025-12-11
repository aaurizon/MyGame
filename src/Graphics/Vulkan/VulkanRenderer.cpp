#include "VulkanRenderer.h"

#include <stdexcept>

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::initialize(void* nativeWindow, int width, int height) {
    hwnd_ = static_cast<HWND>(nativeWindow);
    width_ = width;
    height_ = height;

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
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    hwnd_ = nullptr;
}

void VulkanRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void VulkanRenderer::setWorld(AWorld*) {
    // World data is not consumed in the placeholder renderer yet.
}

void VulkanRenderer::draw(const AViewport&) {
    // Placeholder: until Vulkan pipeline is added, clear the window with a simple color using GDI.
    if (!hwnd_) {
        return;
    }
    HDC hdc = GetDC(hwnd_);
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    HBRUSH brush = CreateSolidBrush(RGB(18, 22, 28));
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    ReleaseDC(hwnd_, hdc);
}
