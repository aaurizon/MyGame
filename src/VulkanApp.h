#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <Volk/volk.h>

#include <optional>
#include <string>
#include <vector>

class Win32Window;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanApp
{
public:
    VulkanApp() = default;
    ~VulkanApp();

    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;

    bool Initialize(const char* appName, const Win32Window& window);
    void Cleanup();

    bool IsInitialized() const { return initialized_; }

    VkInstance GetInstance() const { return instance_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    VkDevice GetDevice() const { return device_; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
    VkQueue GetPresentQueue() const { return presentQueue_; }
    QueueFamilyIndices GetQueueFamilies() const { return queueFamilies_; }
    const std::string& GetSelectedGpuName() const { return selectedGpuName_; }

private:
    bool CreateInstance(const char* appName);
    bool SetupDebugMessenger();
    bool CreateSurface(const Win32Window& window);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    bool CheckValidationLayerSupport() const;
    std::vector<const char*> GatherRequiredExtensions();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    bool initialized_ = false;
    bool validationEnabled_ = false;
    bool debugUtilsEnabled_ = false;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilies_{};
    std::string selectedGpuName_;

    const std::vector<const char*> validationLayers_{"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions_{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
