#include "VkContext.h"

bool VkContext::createInstance()
{
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
                                  .setApiVersion(VK_API_VERSION_1_4)
                                  .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                                  .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                                  .setPApplicationName("Cacao Vulkan Renderer")
                                  .setPEngineName("Cacao Engine");
    std::vector<const char*> enabledLayerNames;
    if (m_graphicsContextCreateInfo.enableValidationLayers)
    {
        enabledLayerNames.push_back("VK_LAYER_KHRONOS_validation");
    }
    uint32_t extensionCount;
    if (vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != vk::Result::eSuccess)
    {
        return false;
    }
    std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
    if (vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) !=
        vk::Result::eSuccess)
    {
        return false;
    }
    std::vector<const char*> extensions;
    for (const auto& extension : availableExtensions)
    {
        extensions.push_back(extension.extensionName);
    }
    vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo().setPEnabledLayerNames(enabledLayerNames)
                                                                        .setEnabledLayerCount(
                                                                            static_cast<uint32_t>(enabledLayerNames.
                                                                                size()))
                                                                        .setPApplicationInfo(&appInfo)
                                                                        .setEnabledExtensionCount(
                                                                            static_cast<uint32_t>(extensions.
                                                                                size()))
                                                                        .setPpEnabledExtensionNames(
                                                                            extensions.data());
    m_instance = vk::createInstance(instanceCreateInfo);
    if (!m_instance)
    {
        throw std::runtime_error("failed to create instance!");
        return false;
    }
    return true;
}

bool VkContext::pickPhysicalDevice()
{
    auto devices = m_instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
        return false;
    }
    m_physicalDevice = devices[0];
    return true;
}

bool VkContext::createSurface()
{
#ifdef WIN32
    vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR().setHwnd(
            m_graphicsContextCreateInfo.nativeWindowHandle.hWnd).
        setHinstance(
            m_graphicsContextCreateInfo.nativeWindowHandle.hInstance);
#endif

    m_surface = m_instance.createWin32SurfaceKHR(surfaceCreateInfo);
    if (!m_surface)
    {
        throw std::runtime_error("failed to create window surface!");
        return false;
    }
    return true;
}

bool VkContext::createDevice()
{
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

    int idx = 0;
    for (auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            graphicsFamily = idx;
        }
        if (m_physicalDevice.getSurfaceSupportKHR(idx, m_surface))
        {
            presentFamily = idx;
        }
        idx++;
    }
    if (graphicsFamily == -1 || presentFamily == -1)
    {
        throw std::runtime_error("failed to find suitable queue families!");
        return false;
    }
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    if (presentFamily != graphicsFamily)
    {
        vk::DeviceQueueCreateInfo presentQueueInfo = vk::DeviceQueueCreateInfo()
                                                     .setQueueFamilyIndex(presentFamily)
                                                     .setQueueCount(1)
                                                     .setPQueuePriorities(&queuePriority);
        deviceQueueCreateInfos.push_back(presentQueueInfo);
    }
    vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
                                                .setQueueCount(1)
                                                .setQueueFamilyIndex(graphicsFamily)
                                                .setPQueuePriorities(&queuePriority);
    deviceQueueCreateInfos.push_back(queueCreateInfo);
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
    };
    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature =
        vk::PhysicalDeviceDynamicRenderingFeatures().setDynamicRendering(true);
    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
                                            .setEnabledExtensionCount(deviceExtensions.size()).
                                            setPpEnabledExtensionNames(deviceExtensions.data()).
                                            setQueueCreateInfoCount(
                                                static_cast<uint32_t>(deviceQueueCreateInfos.size()))
                                            .setPQueueCreateInfos(deviceQueueCreateInfos.data())
                                            .setPNext(&dynamicRenderingFeature);
    if (m_device = m_physicalDevice.createDevice(deviceCreateInfo); !m_device)
    {
        throw std::runtime_error("failed to create logical device!");
        return false;
    }
    m_graphicsQueue = m_device.getQueue(graphicsFamily, 0);
    m_presentQueue = m_device.getQueue(presentFamily, 0);
    return true;
}

bool VkContext::createSwapChain()
{
    vk::SurfaceCapabilitiesKHR capabilities =
        m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

    std::vector<vk::SurfaceFormatKHR> formats =
        m_physicalDevice.getSurfaceFormatsKHR(m_surface);

    std::vector<vk::PresentModeKHR> presentModes =
        m_physicalDevice.getSurfacePresentModesKHR(m_surface);

    for (auto& format : formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            m_surfaceFormat = format;
            break;
        }
        else if (format.format == vk::Format::eR8G8B8A8Unorm &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            m_surfaceFormat = format;
        }
    }
    vk::PresentModeKHR selectedPresentMode = vk::PresentModeKHR::eMailbox;
    if (m_graphicsContextCreateInfo.vsyncEnabled)
    {
        selectedPresentMode = vk::PresentModeKHR::eMailbox;
    }
    else
    {
        selectedPresentMode = vk::PresentModeKHR::eImmediate;
    }
    vk::Extent2D swapchainExtent = {
        m_graphicsContextCreateInfo.windowSize.width,
        m_graphicsContextCreateInfo.windowSize.height
    };
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }
    vk::SharingMode sharingMode;
    std::vector<uint32_t> queueFamilyIndices;
    if (presentFamily != graphicsFamily)
    {
        sharingMode = vk::SharingMode::eConcurrent;
        queueFamilyIndices.push_back(static_cast<uint32_t>(graphicsFamily));
        queueFamilyIndices.push_back(presentFamily);
    }
    else
    {
        sharingMode = vk::SharingMode::eExclusive;
    }
    vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
                                                     .setSurface(m_surface)
                                                     .setMinImageCount(imageCount)
                                                     .setImageFormat(m_surfaceFormat.format)
                                                     .setImageColorSpace(m_surfaceFormat.colorSpace)
                                                     .setImageExtent(swapchainExtent)
                                                     .setImageArrayLayers(1)
                                                     .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                                     .setImageSharingMode(sharingMode)
                                                     .setQueueFamilyIndexCount(
                                                         static_cast<uint32_t>(queueFamilyIndices.size()))
                                                     .setPQueueFamilyIndices(queueFamilyIndices.data())
                                                     .setPreTransform(capabilities.currentTransform)
                                                     .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                                                     .setPresentMode(selectedPresentMode)
                                                     .setClipped(true)
                                                     .setOldSwapchain(nullptr);
    if (m_swapChain = m_device.createSwapchainKHR(swapchainCreateInfo); !m_swapChain)
    {
        throw std::runtime_error("failed to create swap chain!");
        return false;
    }
    m_swapChainImages = m_device.getSwapchainImagesKHR(m_swapChain);
    for (auto& image : m_swapChainImages)
    {
        vk::ImageViewCreateInfo ivci{};
        ivci.image = image;
        ivci.viewType = vk::ImageViewType::e2D;
        ivci.format = m_surfaceFormat.format;
        ivci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;

        m_swapChainImageViews.push_back(m_device.createImageView(ivci));
    }

    return true;
}

void VkContext::recreateSwapChain()
{
    m_device.waitIdle();
    for (auto& imageView : m_swapChainImageViews)
    {
        m_device.destroyImageView(imageView);
    }
    m_device.destroySwapchainKHR(m_swapChain);
    createSwapChain();
}

VkContext::VkContext() = default;
VkContext::~VkContext() = default;

std::shared_ptr<VkContext> VkContext::Create(const GraphicsContextCreateInfo& createInfo)
{
    auto context = std::make_shared<VkContext>();
    if (!context->Initialize(createInfo))
    {
        return nullptr;
    }
    return context;
}

bool VkContext::Initialize(const GraphicsContextCreateInfo& createInfo)
{
    m_graphicsContextCreateInfo = createInfo;
    if (!createInstance())
    {
        return false;
    }
    if (!pickPhysicalDevice())
    {
        return false;
    }
    if (!createSurface())
    {
        return false;
    }
    if (!createDevice())
    {
        return false;
    }
    if (!createSwapChain())
    {
        return false;
    }
    return true;
}

void VkContext::Resize(uint32_t width, uint32_t height)
{
    m_graphicsContextCreateInfo.windowSize.width = width;
    m_graphicsContextCreateInfo.windowSize.height = height;
    recreateSwapChain();
}

vk::Queue& VkContext::GetGraphicsQueue()
{
    return m_graphicsQueue;
}

vk::Queue& VkContext::GetPresentQueue()
{
    return m_presentQueue;
}

vk::SwapchainKHR& VkContext::GetSwapChain()
{
    return m_swapChain;
}

vk::Device& VkContext::GetDevice()
{
    return m_device;
}

vk::SurfaceFormatKHR& VkContext::GetSurfaceFormat()
{
    return m_surfaceFormat;
}

vk::Extent2D VkContext::GetSwapChainExtent() const
{
    return {m_graphicsContextCreateInfo.windowSize.width, m_graphicsContextCreateInfo.windowSize.height};
}

void VkContext::WaitIdle()
{
    m_device.waitIdle();
}
