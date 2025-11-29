#ifndef CACAO_SYNCHRONIZATIONCONTEXT_H
#define CACAO_SYNCHRONIZATIONCONTEXT_H

#include <vulkan/vulkan.hpp>

#include "VkContext.h"

class VkContext;

class SynchronizationContext
{
    uint32_t m_maxFramesInFlight;
    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;
    std::shared_ptr<VkContext> m_vkContext;

public:
    SynchronizationContext();
    ~SynchronizationContext();

    static std::shared_ptr<SynchronizationContext> Create(const std::shared_ptr<VkContext>& context,
                                                          uint32_t maxFramesInFlight);

    bool Initialize(const std::shared_ptr<VkContext>& context, uint32_t maxFramesInFlight);

    void WaitForFrame(uint32_t frameIndex);

    void ResetFrameFence(uint32_t frameIndex);

    uint32_t AcquireNextImageIndex(uint32_t frameIndex) const;

    vk::Semaphore &GetImageSemaphore(uint32_t frameIndex);
    vk::Semaphore &GetRenderSemaphore(uint32_t frameIndex);
    vk::Fence &GetInFlightFence(uint32_t frameIndex);

};
#endif //CACAO_SYNCHRONIZATIONCONTEXT_H
