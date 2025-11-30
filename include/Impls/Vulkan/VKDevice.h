#ifndef CACAO_VKDEVICE_H
#define CACAO_VKDEVICE_H
#include <queue>
#include <vulkan/vulkan.hpp>
#include "CacaoDevice.h"
#include <vk_mem_alloc.h>
namespace Cacao
{
    class VKCommandBufferEncoder;
}
namespace Cacao
{
    class CacaoAdapter;
    class CACAO_API VKDevice final : public CacaoDevice
    {
        vk::Device m_Device;
        friend class VKSynchronization;
        friend class VKSwapchain;
        friend class VKQueue;
        friend class VKBuffer;
        friend class VKTexture;
        friend class VKSurface;
        vk::Device& GetVulkanDevice() { return m_Device; }
        std::vector<uint32_t> m_queueFamilyIndices;
        Ref<CacaoAdapter> m_parentAdapter;
        vk::CommandPool m_graphicsCommandPool;
        std::queue<Ref<VKCommandBufferEncoder>> m_primCommandBuffers;
        std::queue<Ref<VKCommandBufferEncoder>> m_secCommandBuffers;
        VmaAllocator m_allocator;
        VmaAllocator& GetVmaAllocator() { return m_allocator; }
    public:
        static Ref<VKDevice> Create(const Ref<CacaoAdapter>& adapter, const CacaoDeviceCreateInfo& createInfo);
        VKDevice(const Ref<CacaoAdapter>& adapter, const CacaoDeviceCreateInfo& createInfo);
        void WaitIdle() const override;
        Ref<CacaoQueue> GetQueue(QueueType type, uint32_t index) override;
        Ref<CacaoSwapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) override;
        std::vector<uint32_t> GetAllQueueFamilyIndices() const override;
        Ref<CacaoAdapter> GetParentAdapter() const override;
        Ref<CacaoCommandBufferEncoder> CreateCommandBufferEncoder(
            CommandBufferType type = CommandBufferType::Primary) override;
        void ResetCommandPool() override;
        void ReturnCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) override;
        void FreeCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) override;
        void ResetCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) override;
        Ref<CacaoTexture> CreateTexture(const TextureCreateInfo& createInfo) override;
        Ref<CacaoBuffer> CreateBuffer(const BufferCreateInfo& createInfo) override;
    };
}
#endif 
