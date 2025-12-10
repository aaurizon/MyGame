#define VOLK_IMPLEMENTATION
#include "VulkanApp.h"

#include "Win32Window.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <limits>

VulkanApp::~VulkanApp()
{
    Cleanup();
}

bool VulkanApp::Initialize(const char* appName, const Win32Window& window)
{
    if (initialized_)
    {
        return true;
    }

    VkResult volkRes = volkInitialize();
    if (volkRes != VK_SUCCESS)
    {
        std::cerr << "volkInitialize failed: " << volkRes << std::endl;
        return false;
    }

#if defined(NDEBUG)
    validationEnabled_ = false;
#else
    validationEnabled_ = CheckValidationLayerSupport();
    if (!validationEnabled_)
    {
        std::cerr << "Validation layers requested but not available; continuing without them." << std::endl;
    }
#endif

    if (!CreateInstance(appName))
    {
        return false;
    }

    volkLoadInstance(instance_);

    if (!SetupDebugMessenger())
    {
        std::cerr << "Debug messenger setup failed; continuing without it." << std::endl;
    }

    if (!CreateSurface(window))
    {
        return false;
    }

    if (!PickPhysicalDevice())
    {
        return false;
    }

    if (!CreateLogicalDevice())
    {
        return false;
    }

    volkLoadDevice(device_);

    if (!CreateSwapchain(window))
    {
        return false;
    }

    if (!CreateImageViews())
    {
        return false;
    }

    initialized_ = true;
    std::cout << "Vulkan initialized through logical device creation." << std::endl;
    return true;
}

void VulkanApp::Cleanup()
{
    for (VkImageView view : swapchainImageViews_)
    {
        vkDestroyImageView(device_, view, nullptr);
    }
    swapchainImageViews_.clear();

    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (debugMessenger_ != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        debugMessenger_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    initialized_ = false;
}

bool VulkanApp::CreateInstance(const char* appName)
{
    auto extensions = GatherRequiredExtensions();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "MyGameEngine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationEnabled_)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();

        if (debugUtilsEnabled_)
        {
            debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                 void*) -> VkBool32
            {
                const char* severity = "INFO";
                if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    severity = "ERROR";
                }
                else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    severity = "WARNING";
                }
                else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
                {
                    severity = "VERBOSE";
                }

                std::cerr << "[Vulkan][" << severity << "] " << callbackData->pMessage << std::endl;
                return VK_FALSE;
            };
            createInfo.pNext = &debugCreateInfo;
        }
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "vkCreateInstance failed: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanApp::SetupDebugMessenger()
{
    if (!validationEnabled_ || !debugUtilsEnabled_)
    {
        return true;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                    VkDebugUtilsMessageTypeFlagsEXT,
                                    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                    void*) -> VkBool32
    {
        const char* severity = "INFO";
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            severity = "ERROR";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            severity = "WARNING";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            severity = "VERBOSE";
        }

        std::cerr << "[Vulkan][" << severity << "] " << callbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    VkResult result = vkCreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create debug messenger: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanApp::CreateSurface(const Win32Window& window)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = window.GetInstance();
    createInfo.hwnd = window.GetHwnd();

    VkResult result = vkCreateWin32SurfaceKHR(instance_, &createInfo, nullptr, &surface_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "vkCreateWin32SurfaceKHR failed: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanApp::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (result != VK_SUCCESS || deviceCount == 0)
    {
        std::cerr << "Failed to find GPUs with Vulkan support." << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    std::cout << "GPUs detected:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(devices[i], &props);
        bool extensionsOk = CheckDeviceExtensionSupport(devices[i]);
        auto indices = FindQueueFamilies(devices[i]);
        bool queuesOk = indices.IsComplete();
        SwapChainSupportDetails swapSupport = QuerySwapChainSupport(devices[i]);
        bool swapchainOk = !swapSupport.formats.empty() && !swapSupport.presentModes.empty();

        std::cout << "  [" << i << "] " << props.deviceName
                  << " | extensions: " << (extensionsOk ? "ok" : "missing")
                  << " | queues: " << (queuesOk ? "ok" : "missing")
                  << " | swapchain: " << (swapchainOk ? "ok" : "missing")
                  << std::endl;

        if (extensionsOk && queuesOk && swapchainOk && physicalDevice_ == VK_NULL_HANDLE)
        {
            physicalDevice_ = devices[i];
            queueFamilies_ = indices;
            selectedGpuName_ = props.deviceName;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE)
    {
        std::cerr << "No suitable GPU found that supports required queues and extensions." << std::endl;
        return false;
    }

    std::cout << "Selected GPU: " << selectedGpuName_ << std::endl;
    return true;
}

bool VulkanApp::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<uint32_t> uniqueQueues;

    uniqueQueues.push_back(queueFamilies_.graphicsFamily.value());
    if (queueFamilies_.presentFamily != queueFamilies_.graphicsFamily)
    {
        uniqueQueues.push_back(queueFamilies_.presentFamily.value());
    }

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueues)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions_.data();

    if (validationEnabled_)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    }

    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "vkCreateDevice failed: " << result << std::endl;
        return false;
    }

    vkGetDeviceQueue(device_, queueFamilies_.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.presentFamily.value(), 0, &presentQueue_);

    return true;
}

bool VulkanApp::CreateSwapchain(const Win32Window& window)
{
    SwapChainSupportDetails support = QuerySwapChainSupport(physicalDevice_);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
    VkExtent2D extent = ChooseSwapExtent(support.capabilities, window);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {queueFamilies_.graphicsFamily.value(), queueFamilies_.presentFamily.value()};

    if (queueFamilies_.graphicsFamily != queueFamilies_.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create swapchain: " << result << std::endl;
        return false;
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    swapchainImageFormat_ = surfaceFormat.format;
    swapchainExtent_ = extent;

    std::cout << "Swapchain created with " << imageCount << " images, format "
              << swapchainImageFormat_ << " extent " << extent.width << "x" << extent.height << std::endl;
    return true;
}

bool VulkanApp::CreateImageViews()
{
    swapchainImageViews_.resize(swapchainImages_.size());

    for (size_t i = 0; i < swapchainImages_.size(); ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device_, &createInfo, nullptr, &swapchainImageViews_[i]);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create image view for swapchain image " << i << ": " << result << std::endl;
            return false;
        }
    }

    return true;
}

bool VulkanApp::CheckValidationLayerSupport() const
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers_)
    {
        bool found = std::any_of(availableLayers.begin(), availableLayers.end(),
                                 [layerName](const VkLayerProperties& props)
                                 {
                                     return std::strcmp(layerName, props.layerName) == 0;
                                 });
        if (!found)
        {
            return false;
        }
    }
    return true;
}

std::vector<const char*> VulkanApp::GatherRequiredExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    auto extensionAvailable = [&extensions](const char* name)
    {
        return std::any_of(extensions.begin(), extensions.end(),
                           [name](const VkExtensionProperties& ext)
                           {
                               return std::strcmp(ext.extensionName, name) == 0;
                           });
    };

    std::vector<const char*> required{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    debugUtilsEnabled_ = validationEnabled_ && extensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (debugUtilsEnabled_)
    {
        required.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    else if (validationEnabled_)
    {
        std::cerr << "VK_EXT_debug_utils not available; debug messenger will be skipped." << std::endl;
    }

    return required;
}

bool VulkanApp::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());

    for (const char* required : deviceExtensions_)
    {
        bool found = std::any_of(available.begin(), available.end(),
                                 [required](const VkExtensionProperties& prop)
                                 {
                                     return std::strcmp(required, prop.extensionName) == 0;
                                 });
        if (!found)
        {
            return false;
        }
    }
    return true;
}

QueueFamilyIndices VulkanApp::FindQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.IsComplete())
        {
            break;
        }
    }

    return indices;
}

VulkanApp::SwapChainSupportDetails VulkanApp::QuerySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanApp::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats.empty() ? VkSurfaceFormatKHR{} : availableFormats[0];
}

VkPresentModeKHR VulkanApp::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
{
    for (const auto& presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Win32Window& window) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    WindowSize size = window.GetClientSize();
    VkExtent2D actualExtent = {size.width, size.height};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
