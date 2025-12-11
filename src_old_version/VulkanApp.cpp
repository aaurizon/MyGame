#define VOLK_IMPLEMENTATION
#include "../src/VulkanApp.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vma/vk_mem_alloc.h>

#include "../src/Win32Window.h"

#include <cstddef>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <limits>
#include <array>

struct Vertex
{
    float pos[2];
    float color[3];
};

static VkVertexInputBindingDescription GetBindingDescription()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attrs{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, color);
    return attrs;
}

static const std::array<Vertex, 3> kVertices = {{
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
}};

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

    if (!CreateAllocator())
    {
        return false;
    }

    if (!CreateSwapchain(window))
    {
        return false;
    }

    if (!CreateImageViews())
    {
        return false;
    }

    if (!CreateRenderPass())
    {
        return false;
    }

    if (!CreateGraphicsPipeline())
    {
        return false;
    }

    if (!CreateFramebuffers())
    {
        return false;
    }

    if (!CreateCommandPool())
    {
        return false;
    }

    if (!CreateVertexBuffer())
    {
        return false;
    }

    if (!CreateCommandBuffers())
    {
        return false;
    }

    if (!CreateSyncObjects())
    {
        return false;
    }

    initialized_ = true;
    std::cout << "Vulkan initialized through pipeline and framebuffers." << std::endl;
    return true;
}

void VulkanApp::Cleanup()
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);

        for (size_t i = 0; i < imageAvailableSemaphores_.size(); ++i)
        {
            vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
        }
        imageAvailableSemaphores_.clear();

        for (size_t i = 0; i < renderFinishedSemaphores_.size(); ++i)
        {
            vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
        }
        renderFinishedSemaphores_.clear();

        for (VkFence fence : inFlightFences_)
        {
            vkDestroyFence(device_, fence, nullptr);
        }
        inFlightFences_.clear();
        imagesInFlight_.clear();

        if (commandPool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
        }

        if (vertexBuffer_ != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator_, vertexBuffer_, vertexAllocation_);
            vertexBuffer_ = VK_NULL_HANDLE;
            vertexAllocation_ = VK_NULL_HANDLE;
        }

        if (allocator_ != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(allocator_);
            allocator_ = VK_NULL_HANDLE;
        }

        for (VkFramebuffer fb : framebuffers_)
        {
            vkDestroyFramebuffer(device_, fb, nullptr);
        }
        framebuffers_.clear();

        if (graphicsPipeline_ != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
            graphicsPipeline_ = VK_NULL_HANDLE;
        }

        if (pipelineLayout_ != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }

        if (renderPass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device_, renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }

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

bool VulkanApp::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanApp::CreateGraphicsPipeline()
{
    // Precompiled SPIR-V for a basic vertex-color pipeline.
    static const std::vector<uint32_t> vertexShaderSpv = {
        0x07230203,0x00010000,0x000d000b,0x00000021,0x00000000,0x00020011,0x00000001,0x0006000b,0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000012,0x0000001d,0x0000001f,0x00030003,0x00000002,0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,0x5f656c79,0x656e696c,0x7269645f,0x69746365,0x00006576,0x00080004,0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,0x69645f65,0x74636572,0x00657669,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,0x0000000b,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000000b,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000000b,0x00000001,0x505f6c67,0x746e696f,0x657a6953,0x00000000,0x00070006,0x0000000b,0x00000002,0x435f6c67,0x4470696c,0x61747369,0x0065636e,0x00070006,0x0000000b,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,0x00030005,0x0000000d,0x00000000,0x00040005,0x00000012,0x6f506e69,0x00000073,0x00050005,0x0000001d,0x67617266,0x6f6c6f43,0x00000072,0x00040005,0x0000001f,0x6f436e69,0x00726f6c,0x00030047,0x0000000b,0x00000002,0x00050048,0x0000000b,0x00000000,0x0000000b,0x00000000,0x00050048,0x0000000b,0x00000001,0x0000000b,0x00000001,0x00050048,0x0000000b,0x00000002,0x0000000b,0x00000003,0x00050048,0x0000000b,0x00000003,0x0000000b,0x00000004,0x00040047,0x00000012,0x0000001e,0x00000000,0x00040047,0x0000001d,0x0000001e,0x00000000,0x00040047,0x0000001f,0x0000001e,0x00000001,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,0x00000008,0x00000009,0x00000001,0x0004001c,0x0000000a,0x00000006,0x00000009,0x0006001e,0x0000000b,0x00000007,0x00000006,0x0000000a,0x0000000a,0x00040020,0x0000000c,0x00000003,0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000003,0x00040015,0x0000000e,0x00000020,0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040017,0x00000010,0x00000006,0x00000002,0x00040020,0x00000011,0x00000001,0x00000010,0x0004003b,0x00000011,0x00000012,0x00000001,0x0004002b,0x00000006,0x00000014,0x00000000,0x0004002b,0x00000006,0x00000015,0x3f800000,0x00040020,0x00000019,0x00000003,0x00000007,0x00040017,0x0000001b,0x00000006,0x00000003,0x00040020,0x0000001c,0x00000003,0x0000001b,0x0004003b,0x0000001c,0x0000001d,0x00000003,0x00040020,0x0000001e,0x00000001,0x0000001b,0x0004003b,0x0000001e,0x0000001f,0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x00000010,0x00000013,0x00000012,0x00050051,0x00000006,0x00000016,0x00000013,0x00000000,0x00050051,0x00000006,0x00000017,0x00000013,0x00000001,0x00070050,0x00000007,0x00000018,0x00000016,0x00000017,0x00000014,0x00000015,0x00050041,0x00000019,0x0000001a,0x0000000d,0x0000000f,0x0003003e,0x0000001a,0x00000018,0x0004003d,0x0000001b,0x00000020,0x0000001f,0x0003003e,0x0000001d,0x00000020,0x000100fd,0x00010038};

    static const std::vector<uint32_t> fragmentShaderSpv = {
        0x07230203,0x00010000,0x000d000b,0x00000013,0x00000000,0x00020011,0x00000001,0x0006000b,0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000c,0x00030010,0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,0x5f656c79,0x656e696c,0x7269645f,0x69746365,0x00006576,0x00080004,0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,0x69645f65,0x74636572,0x00657669,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00050005,0x0000000c,0x67617266,0x6f6c6f43,0x00000072,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000c,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,0x00000003,0x00040020,0x0000000b,0x00000001,0x0000000a,0x0004003b,0x0000000b,0x0000000c,0x00000001,0x0004002b,0x00000006,0x0000000e,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x0000000a,0x0000000d,0x0000000c,0x00050051,0x00000006,0x0000000f,0x0000000d,0x00000000,0x00050051,0x00000006,0x00000010,0x0000000d,0x00000001,0x00050051,0x00000006,0x00000011,0x0000000d,0x00000002,0x00070050,0x00000007,0x00000012,0x0000000f,0x00000010,0x00000011,0x0000000e,0x0003003e,0x00000009,0x00000012,0x000100fd,0x00010038};

    VkShaderModule vertShaderModule = CreateShaderModule(
        std::vector<uint32_t>(vertexShaderSpv.begin(), vertexShaderSpv.end()));
    VkShaderModule fragShaderModule = CreateShaderModule(
        std::vector<uint32_t>(fragmentShaderSpv.begin(), fragmentShaderSpv.end()));

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription = GetBindingDescription();
    auto attributeDescriptions = GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline layout." << std::endl;
        vkDestroyShaderModule(device_, vertShaderModule, nullptr);
        vkDestroyShaderModule(device_, fragShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
    vkDestroyShaderModule(device_, fragShaderModule, nullptr);

    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create graphics pipeline: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanApp::CreateFramebuffers()
{
    framebuffers_.resize(swapchainImageViews_.size());

    for (size_t i = 0; i < swapchainImageViews_.size(); ++i)
    {
        VkImageView attachments[] = {swapchainImageViews_[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffers_[i]);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create framebuffer " << i << ": " << result << std::endl;
            return false;
        }
    }

    return true;
}

bool VulkanApp::CreateAllocator()
{
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;

    VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create VMA allocator: " << result << std::endl;
        return false;
    }
    return true;
}

bool VulkanApp::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(kVertices[0]) * kVertices.size();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocationInfo{};
    VkResult result = vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &vertexBuffer_, &vertexAllocation_, &allocationInfo);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create vertex buffer: " << result << std::endl;
        return false;
    }

    std::memcpy(allocationInfo.pMappedData, kVertices.data(), static_cast<size_t>(bufferSize));
    return true;
}

bool VulkanApp::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilies_.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create command pool: " << result << std::endl;
        return false;
    }
    return true;
}

bool VulkanApp::CreateCommandBuffers()
{
    if (!commandBuffers_.empty())
    {
        vkFreeCommandBuffers(device_, commandPool_, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
    }

    commandBuffers_.resize(swapchainImages_.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    VkResult result = vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data());
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate command buffers: " << result << std::endl;
        return false;
    }

    for (uint32_t i = 0; i < commandBuffers_.size(); ++i)
    {
        RecordCommandBuffer(commandBuffers_[i], i);
    }

    return true;
}

bool VulkanApp::CreateSyncObjects()
{
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(swapchainImages_.size());
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight_.resize(swapchainImages_.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create synchronization objects for frame " << i << std::endl;
            return false;
        }
    }

    for (size_t i = 0; i < renderFinishedSemaphores_.size(); ++i)
    {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create renderFinished semaphore for image " << i << std::endl;
            return false;
        }
    }

    return true;
}

void VulkanApp::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = framebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;

    VkClearValue clearColor{};
    clearColor.color = {{0.02f, 0.02f, 0.05f, 1.0f}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    VkBuffer vertexBuffers[] = {vertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(commandBuffer, static_cast<uint32_t>(kVertices.size()), 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

bool VulkanApp::DrawFrame(Win32Window& window)
{
    WindowSize size = window.GetClientSize();
    if (size.width == 0 || size.height == 0)
    {
        return true; // minimized; skip rendering
    }

    if (window.ConsumeResizeFlag())
    {
        if (!RecreateSwapchain(window))
        {
            return false;
        }
    }

    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                                   imageAvailableSemaphores_[currentFrame_],
                                                   VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (!RecreateSwapchain(window))
        {
            return false;
        }
        return true;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Failed to acquire swapchain image: " << acquireResult << std::endl;
        return false;
    }

    if (imagesInFlight_[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device_, 1, &imagesInFlight_[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight_[imageIndex] = inFlightFences_[currentFrame_];

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    vkResetCommandBuffer(commandBuffers_[imageIndex], 0);
    RecordCommandBuffer(commandBuffers_[imageIndex], imageIndex);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[imageIndex]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS)
    {
        std::cerr << "Failed to submit draw command buffer." << std::endl;
        return false;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        if (!RecreateSwapchain(window))
        {
            return false;
        }
    }
    else if (presentResult != VK_SUCCESS)
    {
        std::cerr << "Failed to present swapchain image: " << presentResult << std::endl;
        return false;
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

VkShaderModule VulkanApp::CreateShaderModule(const std::vector<uint32_t>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        std::cerr << "Failed to create shader module." << std::endl;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void VulkanApp::CleanupSwapchain()
{
    vkDeviceWaitIdle(device_);

    for (VkFramebuffer fb : framebuffers_)
    {
        vkDestroyFramebuffer(device_, fb, nullptr);
    }
    framebuffers_.clear();

    if (!commandBuffers_.empty())
    {
        vkFreeCommandBuffers(device_, commandPool_, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
        commandBuffers_.clear();
    }

    if (graphicsPipeline_ != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
        graphicsPipeline_ = VK_NULL_HANDLE;
    }

    if (pipelineLayout_ != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    if (renderPass_ != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

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

    for (VkSemaphore sem : renderFinishedSemaphores_)
    {
        vkDestroySemaphore(device_, sem, nullptr);
    }
    renderFinishedSemaphores_.clear();
}

bool VulkanApp::RecreateSwapchain(const Win32Window& window)
{
    WindowSize size = window.GetClientSize();
    while (size.width == 0 || size.height == 0)
    {
        size = window.GetClientSize();
    }

    CleanupSwapchain();

    if (!CreateSwapchain(window))
    {
        return false;
    }

    if (!CreateImageViews())
    {
        return false;
    }

    if (!CreateRenderPass())
    {
        return false;
    }

    if (!CreateGraphicsPipeline())
    {
        return false;
    }

    if (!CreateFramebuffers())
    {
        return false;
    }

    if (!CreateCommandBuffers())
    {
        return false;
    }

    imagesInFlight_.assign(swapchainImages_.size(), VK_NULL_HANDLE);

    // Recreate render-finished semaphores sized to swapchain images.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    renderFinishedSemaphores_.resize(swapchainImages_.size());
    for (size_t i = 0; i < renderFinishedSemaphores_.size(); ++i)
    {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to recreate renderFinished semaphore for image " << i << std::endl;
            return false;
        }
    }

    return true;
}
