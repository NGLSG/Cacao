#ifndef CACAO_VKSYNCHRONIZATION_H
#define CACAO_VKSYNCHRONIZATION_H
#include "CacaoSynchronization.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class VKDevice;
    class CACAO_API VKSynchronization : public CacaoSynchronization
    {
        uint32_t m_maxFramesInFlight;
        std::vector<vk::Semaphore> m_imageAvailableSemaphores;
        std::vector<vk::Semaphore> m_renderFinishedSemaphores;
        std::vector<vk::Fence> m_inFlightFences;
        Ref<VKDevice> m_vkDevice;
        friend class VKDevice;
        friend class VKSwapchain;
        friend class VKQueue;
        vk::Semaphore& GetImageSemaphore(uint32_t frameIndex);
        vk::Semaphore& GetRenderSemaphore(uint32_t frameIndex);
        vk::Fence& GetInFlightFence(uint32_t frameIndex);
    public:
        static Ref<VKSynchronization> Create(const Ref<VKDevice>& device, uint32_t maxFramesInFlight);
        VKSynchronization(const Ref<VKDevice>& device, uint32_t maxFramesInFlight);
        void WaitForFrame(uint32_t frameIndex) const override;
        void ResetFrameFence(uint32_t frameIndex) const override;
        uint32_t AcquireNextImageIndex(const Ref<CacaoSwapchain>& swapchain, uint32_t frameIndex) const override;
        uint32_t GetMaxFramesInFlight() const override;
    };
} 
#endif 
