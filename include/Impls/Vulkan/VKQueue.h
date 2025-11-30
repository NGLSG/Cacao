#ifndef CACAO_VKQUEUE_H
#define CACAO_VKQUEUE_H
#include <mutex>
#include <vulkan/vulkan.hpp>
#include "CacaoQueue.h"
namespace Cacao
{
    class CacaoDevice;
}
namespace Cacao
{
    class CACAO_API VKQueue final : public CacaoQueue
    {
        vk::Queue m_queue;
        Ref<CacaoDevice> m_device;
        QueueType m_type;
        uint32_t m_index;
        std::mutex m_submitMutex;
        friend class VKSwapchain;
        vk::Queue& GetVulkanQueue()
        {
            return m_queue;
        }
    public:
        VKQueue(const Ref<CacaoDevice>& device, const vk::Queue& vkQueue, QueueType type, uint32_t index);
        static Ref<VKQueue> Create(const Ref<CacaoDevice>& device, const vk::Queue& vkQueue, QueueType type,
                                   uint32_t index);
        QueueType GetType() const override;
        uint32_t GetIndex() const override;
        void Submit(const Ref<CacaoCommandBufferEncoder>& cmd, const Ref<CacaoSynchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(const std::vector<Ref<CacaoCommandBufferEncoder>>& cmds, const Ref<CacaoSynchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(const Ref<CacaoCommandBufferEncoder>& cmd) override;
        void WaitIdle() const override;
    };
}
#endif 
