#include "Impls/Vulkan/VKSynchronization.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKSwapchain.h"
namespace Cacao
{
    vk::Semaphore& VKSynchronization::GetImageSemaphore(uint32_t frameIndex)
    {
        return m_imageAvailableSemaphores[frameIndex];
    }
    vk::Semaphore& VKSynchronization::GetRenderSemaphore(uint32_t frameIndex)
    {
        return m_renderFinishedSemaphores[frameIndex];
    }
    vk::Fence& VKSynchronization::GetInFlightFence(uint32_t frameIndex)
    {
        return m_inFlightFences[frameIndex];
    }
    Ref<VKSynchronization> VKSynchronization::Create(const Ref<VKDevice>& device, uint32_t maxFramesInFlight)
    {
        auto sync = CreateRef<VKSynchronization>(device, maxFramesInFlight);
        return sync;
    }
    VKSynchronization::VKSynchronization(const Ref<VKDevice>& device, uint32_t maxFramesInFlight) : m_vkDevice(device),
        m_maxFramesInFlight(maxFramesInFlight)
    {
        if (!device)
        {
            throw std::runtime_error("VKSynchronization created with null device");
        }
        m_inFlightFences.resize(maxFramesInFlight);
        m_renderFinishedSemaphores.resize(maxFramesInFlight);
        m_imageAvailableSemaphores.resize(maxFramesInFlight);
        vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
        vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
        for (uint32_t i = 0; i < maxFramesInFlight; ++i)
        {
            m_imageAvailableSemaphores[i] = m_vkDevice->GetVulkanDevice().createSemaphore(semaphoreInfo);
            m_renderFinishedSemaphores[i] = m_vkDevice->GetVulkanDevice().createSemaphore(semaphoreInfo);
            m_inFlightFences[i] = m_vkDevice->GetVulkanDevice().createFence(fenceInfo);
        }
    }
    void VKSynchronization::WaitForFrame(uint32_t frameIndex) const
    {
        if (m_vkDevice->GetVulkanDevice().waitForFences(1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX) !=
            vk::Result::eSuccess)
        {
            throw std::runtime_error("Waiting for fence failed");
        }
    }
    void VKSynchronization::ResetFrameFence(uint32_t frameIndex) const
    {
        if (m_vkDevice->GetVulkanDevice().resetFences(1, &m_inFlightFences[frameIndex]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Resetting fence failed");
        }
    }
    uint32_t VKSynchronization::AcquireNextImageIndex(const Ref<CacaoSwapchain>& swapchain, uint32_t frameIndex) const
    {
        if (!swapchain)
        {
            throw std::runtime_error("AcquireNextImageIndex called with null swapchain");
        }
        return m_vkDevice->GetVulkanDevice().acquireNextImageKHR(
            std::dynamic_pointer_cast<VKSwapchain>(swapchain)->GetVulkanSwapchain(),
            UINT64_MAX,
            m_imageAvailableSemaphores[frameIndex],
            nullptr).value;
    }
    uint32_t VKSynchronization::GetMaxFramesInFlight() const
    {
        return m_maxFramesInFlight;
    }
} 
