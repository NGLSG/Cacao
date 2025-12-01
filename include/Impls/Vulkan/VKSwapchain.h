#ifndef CACAO_VKSWAPCHAIN_H
#define CACAO_VKSWAPCHAIN_H
#include <CacaoSwapchain.h>
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class VKQueue;
    class VKDevice;
    class CacaoDevice;
    class CACAO_API VKSwapchain : public CacaoSwapchain
    {
        vk::SwapchainKHR m_swapchain;
        friend class VKSynchronization;
        vk::SwapchainKHR& GetVulkanSwapchain() { return m_swapchain; }
        std::vector<vk::Image> m_images;
        std::vector<vk::ImageView> m_imageViews;
        Ref<VKDevice> m_device;
        SwapchainCreateInfo m_swapchainCreateInfo;
    public:
        static Ref<VKSwapchain> Create(const Ref<CacaoDevice>& device, const SwapchainCreateInfo& createInfo);
        VKSwapchain(const Ref<CacaoDevice>& device, const SwapchainCreateInfo& createInfo);
        Result Present(const Ref<CacaoQueue>& queue, const Ref<CacaoSynchronization>& sync,
                       uint32_t frameIndex) override;
        uint32_t GetImageCount() const override;
        Ref<CacaoTexture> GetBackBuffer(uint32_t index) const override;
        Extent2D GetExtent() const override;
        Format GetFormat() const override;
        PresentMode GetPresentMode() const override;
        Result AcquireNextImage(const Ref<CacaoSynchronization>& sync, int idx, int& out) override;
        ~VKSwapchain() override;
    };
} 
#endif 
