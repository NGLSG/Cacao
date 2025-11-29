#include "SynchronizationContext.h"

#include "VkContext.h"
SynchronizationContext::SynchronizationContext() = default;
SynchronizationContext::~SynchronizationContext() = default;

std::shared_ptr<SynchronizationContext> SynchronizationContext::Create(const std::shared_ptr<VkContext>& context,
                                                                       uint32_t maxFramesInFlight)
{
    auto syncContext = std::make_shared<SynchronizationContext>();
    if (!syncContext->Initialize(context, maxFramesInFlight))
    {
        return nullptr;
    }
    return syncContext;
}

bool SynchronizationContext::Initialize(const std::shared_ptr<VkContext>& context, uint32_t maxFramesInFlight)
{
    m_maxFramesInFlight = maxFramesInFlight;
    m_vkContext = context;
    m_inFlightFences.resize(maxFramesInFlight);
    m_renderFinishedSemaphores.resize(maxFramesInFlight);
    m_imageAvailableSemaphores.resize(maxFramesInFlight);
    vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
    vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        m_imageAvailableSemaphores[i] = m_vkContext->GetDevice().createSemaphore(semaphoreInfo);
        m_renderFinishedSemaphores[i] = m_vkContext->GetDevice().createSemaphore(semaphoreInfo);
        m_inFlightFences[i] = m_vkContext->GetDevice().createFence(fenceInfo);
    }
    return true;
}

void SynchronizationContext::WaitForFrame(uint32_t frameIndex)
{
    if (m_vkContext->GetDevice().waitForFences(1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX) !=
        vk::Result::eSuccess)
    {
        throw std::runtime_error("Waiting for fence failed");
    }
}

void SynchronizationContext::ResetFrameFence(uint32_t frameIndex)
{
    if (m_vkContext->GetDevice().resetFences(1, &m_inFlightFences[frameIndex]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Resetting fence failed");
    }
}

uint32_t SynchronizationContext::AcquireNextImageIndex(uint32_t frameIndex) const
{
    return m_vkContext->GetDevice().acquireNextImageKHR(
        m_vkContext->GetSwapChain(), UINT64_MAX, m_imageAvailableSemaphores[frameIndex], nullptr).value;
}

vk::Semaphore& SynchronizationContext::GetImageSemaphore(uint32_t frameIndex)
{
    return m_imageAvailableSemaphores[frameIndex];
}

vk::Semaphore& SynchronizationContext::GetRenderSemaphore(uint32_t frameIndex)
{
    return m_renderFinishedSemaphores[frameIndex];
}

vk::Fence& SynchronizationContext::GetInFlightFence(uint32_t frameIndex)
{
    return m_inFlightFences[frameIndex];
}
