#ifndef CACAO_VKSURFACE_H
#define CACAO_VKSURFACE_H
#include "CacaoSurface.h"
#include  <vulkan/vulkan.hpp>
namespace Cacao
{
    class CacaoDevice;
    class VKQueue;
}
namespace Cacao
{
    class CACAO_API VKSurface : public CacaoSurface
    {
        vk::SurfaceKHR m_surface;
        SurfaceCapabilities m_surfaceCapabilities;
        std::vector<SurfaceFormat> m_surfaceFormats;
        std::vector<PresentMode> m_presentModes;
        friend class VKSwapchain;
        vk::SurfaceKHR& GetVulkanSurface() { return m_surface; }
    public:
        VKSurface(const vk::SurfaceKHR& surface);
        SurfaceCapabilities GetCapabilities(const Ref<CacaoAdapter>& adapter) override;
        std::vector<SurfaceFormat> GetSupportedFormats(const Ref<CacaoAdapter>& adapter) override;
        std::vector<PresentMode> GetSupportedPresentModes(const Ref<CacaoAdapter>& adapter) override;
        uint32_t GetPresentQueueFamilyIndex(const Ref<CacaoAdapter>& adapter) const;
        Ref<CacaoQueue> GetPresentQueue(const Ref<CacaoDevice>& device) override;
    };
}
#endif 
