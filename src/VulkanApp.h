#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk/volk.h>

#include <optional>
#include <string>
#include <vector>

class Win32Window;
struct VmaAllocator_T;
struct VmaAllocation_T;
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

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
    bool DrawFrame(const Win32Window& window);

    bool IsInitialized() const { return initialized_; }

    VkInstance GetInstance() const { return instance_; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
    VkDevice GetDevice() const { return device_; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
    VkQueue GetPresentQueue() const { return presentQueue_; }
    QueueFamilyIndices GetQueueFamilies() const { return queueFamilies_; }
    const std::string& GetSelectedGpuName() const { return selectedGpuName_; }
    VkSwapchainKHR GetSwapchain() const { return swapchain_; }
    VkFormat GetSwapchainImageFormat() const { return swapchainImageFormat_; }
    VkExtent2D GetSwapchainExtent() const { return swapchainExtent_; }
    const std::vector<VkImageView>& GetSwapchainImageViews() const { return swapchainImageViews_; }
    VkRenderPass GetRenderPass() const { return renderPass_; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout_; }
    VkPipeline GetGraphicsPipeline() const { return graphicsPipeline_; }
    const std::vector<VkFramebuffer>& GetFramebuffers() const { return framebuffers_; }

private:
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    bool CreateInstance(const char* appName);
    bool SetupDebugMessenger();
    bool CreateSurface(const Win32Window& window);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain(const Win32Window& window);
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateGraphicsPipeline();
    bool CreateFramebuffers();
    bool CreateAllocator();
    bool CreateVertexBuffer();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    bool RecreateSwapchain(const Win32Window& window);
    void CleanupSwapchain();
    VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code) const;

    bool CheckValidationLayerSupport() const;
    std::vector<const char*> GatherRequiredExtensions();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Win32Window& window) const;

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

    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame_ = 0;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    std::vector<VkFence> imagesInFlight_;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_{};

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    const std::vector<const char*> validationLayers_{"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions_{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
