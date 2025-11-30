#ifndef CACAO_VKCONTEXT_H
#define CACAO_VKCONTEXT_H
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include "windows.h"
struct NativeWindowHandle
{
#ifdef WIN32
    HINSTANCE hInstance;
    HWND hWnd;
#endif
    bool isValid() const
    {
#ifdef WIN32
        return hInstance != nullptr && hWnd != nullptr;
#endif
    }
};
struct GraphicsContextCreateInfo
{
    NativeWindowHandle nativeWindowHandle;
    bool enableValidationLayers;
    bool vsyncEnabled;
    struct
    {
        uint32_t width;
        uint32_t height;
    } windowSize;
};
class VkContext
{
    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_device;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    vk::SurfaceKHR m_surface;
    vk::SwapchainKHR m_swapChain;
    bool m_initialized{};
    GraphicsContextCreateInfo m_graphicsContextCreateInfo;
    vk::SurfaceFormatKHR m_surfaceFormat;
    std::vector<vk::ImageView> m_swapChainImageViews;
    std::vector<vk::Image> m_swapChainImages;
    bool createInstance();
    bool pickPhysicalDevice();
    bool createSurface();
    uint32_t graphicsFamily = -1;
    uint32_t presentFamily = -1;
    bool createDevice();
    bool createSwapChain();
    void recreateSwapChain();
public:
    VkContext();
    ~VkContext();
    static std::shared_ptr<VkContext> Create(const GraphicsContextCreateInfo& createInfo);
    bool Initialize(const GraphicsContextCreateInfo& createInfo);
    void Resize(uint32_t width, uint32_t height);
    vk::Queue& GetGraphicsQueue();
    vk::Queue& GetPresentQueue();
    vk::SwapchainKHR& GetSwapChain();
    vk::Device& GetDevice();
    vk::SurfaceFormatKHR& GetSurfaceFormat();
    vk::Extent2D GetSwapChainExtent() const;
    vk::ImageView& GetSwapChainImageView(size_t index)
    {
        return m_swapChainImageViews[index];
    }
    vk::Image& GetSwapChainImage(size_t index)
    {
        return m_swapChainImages[index];
    }
    uint32_t GetGraphicsFamilyIndex() const
    {
        return graphicsFamily;
    }
    uint32_t GetPresentFamilyIndex() const
    {
        return presentFamily;
    }
    void WaitIdle();
};
#endif 
