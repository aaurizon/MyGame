#define VOLK_IMPLEMENTATION
#include "VulkanApp.h"

#include "Win32Window.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

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

    initialized_ = true;
    std::cout << "Vulkan initialized through logical device creation." << std::endl;
    return true;
}

void VulkanApp::Cleanup()
{
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

        std::cout << "  [" << i << "] " << props.deviceName
                  << " | extensions: " << (extensionsOk ? "ok" : "missing")
                  << " | queues: " << (queuesOk ? "ok" : "missing")
                  << std::endl;

        if (extensionsOk && queuesOk && physicalDevice_ == VK_NULL_HANDLE)
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
