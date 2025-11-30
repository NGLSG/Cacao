#include "Impls/Vulkan/VKQueue.h"
#include <mutex>
#include "Impls/Vulkan/VKCommandBufferEncoder.h"
#include "Impls/Vulkan/VKSynchronization.h"
namespace Cacao
{
    VKQueue::VKQueue(const Ref<CacaoDevice>& device, const vk::Queue& vkQueue, QueueType type, uint32_t index) :
        m_device(device), m_queue(vkQueue), m_type(type), m_index(index)
    {
        if (!m_device)
        {
            throw std::runtime_error("VKQueue created with null device");
        }
        if (!m_queue)
        {
            throw std::runtime_error("VKQueue created with invalid vk::Queue");
        }
    }
    Ref<VKQueue> VKQueue::Create(const Ref<CacaoDevice>& device, const vk::Queue& vkQueue, QueueType type,
                                 uint32_t index)
    {
        return CreateRef<VKQueue>(device, vkQueue, type, index);
    }
    QueueType VKQueue::GetType() const
    {
        return m_type;
    }
    uint32_t VKQueue::GetIndex() const
    {
        return m_index;
    }
    void VKQueue::Submit(const Ref<CacaoCommandBufferEncoder>& cmd, const Ref<CacaoSynchronization>& sync,
                         uint32_t frameIndex)
    {
        auto vkCmd = std::static_pointer_cast<VKCommandBufferEncoder>(cmd);
        auto vkSync = std::static_pointer_cast<VKSynchronization>(sync);
        vk::Semaphore waitSem = vkSync->GetImageSemaphore(frameIndex);
        vk::Semaphore signalSem = vkSync->GetRenderSemaphore(frameIndex);
        vk::Fence fence = vkSync->GetInFlightFence(frameIndex);
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::CommandBuffer buffer = vkCmd->GetVulkanCommandBuffer();
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setWaitSemaphoreCount(1)
                                    .setPWaitSemaphores(&waitSem)
                                    .setPWaitDstStageMask(&waitStage)
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&buffer)
                                    .setSignalSemaphoreCount(1)
                                    .setPSignalSemaphores(&signalSem);
        {
            std::lock_guard<std::mutex> lock(m_submitMutex);
            auto result = m_queue.submit(1, &submitInfo, fence);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to submit queue!");
            }
        }
    }
    void VKQueue::Submit(const std::vector<Ref<CacaoCommandBufferEncoder>>& cmds, const Ref<CacaoSynchronization>& sync,
                         uint32_t frameIndex)
    {
        if (cmds.empty()) return;
        auto vkSync = std::static_pointer_cast<VKSynchronization>(sync);
        std::vector<vk::CommandBuffer> cmdHandles;
        cmdHandles.reserve(cmds.size());
        for (const auto& cmd : cmds)
        {
            auto vkCmd = std::static_pointer_cast<VKCommandBufferEncoder>(cmd);
            cmdHandles.push_back(vkCmd->GetVulkanCommandBuffer());
        }
        vk::Semaphore waitSem = vkSync->GetImageSemaphore(frameIndex);
        vk::Semaphore signalSem = vkSync->GetRenderSemaphore(frameIndex);
        vk::Fence fence = vkSync->GetInFlightFence(frameIndex);
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setWaitSemaphoreCount(1)
                                    .setPWaitSemaphores(&waitSem)
                                    .setPWaitDstStageMask(&waitStage)
                                    .setCommandBufferCount(static_cast<uint32_t>(cmdHandles.size()))
                                    .setPCommandBuffers(cmdHandles.data())
                                    .setSignalSemaphoreCount(1)
                                    .setPSignalSemaphores(&signalSem);
        {
            std::lock_guard<std::mutex> lock(m_submitMutex);
            auto result = m_queue.submit(1, &submitInfo, fence);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to submit queue!");
            }
        }
    }
    void VKQueue::Submit(const Ref<CacaoCommandBufferEncoder>& cmd)
    {
        auto vkCmd = std::static_pointer_cast<VKCommandBufferEncoder>(cmd);
        vk::CommandBuffer cmdHandle = vkCmd->GetVulkanCommandBuffer();
        vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&cmdHandle);
        {
            std::lock_guard<std::mutex> lock(m_submitMutex);
            auto result = m_queue.submit(1, &submitInfo, nullptr);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to submit single command!");
            }
        }
    }
    void VKQueue::WaitIdle() const
    {
        m_queue.waitIdle();
    }
}
