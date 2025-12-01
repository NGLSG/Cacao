#ifndef CACAO_VKDEVICE_H
#define CACAO_VKDEVICE_H
#include <queue>
#include <vulkan/vulkan.hpp>
#include "CacaoDevice.h"
#include <vk_mem_alloc.h>
namespace Cacao
{
    class CacaoAdapter;
    class VKCommandBufferEncoder;
    class CACAO_API VKDevice final : public CacaoDevice
    {
        vk::Device m_Device;
        friend class VKSynchronization;
        friend class VKSwapchain;
        friend class VKQueue;
        friend class VKBuffer;
        friend class VKTexture;
        friend class VKTextureView;
        friend class VKSurface;
        friend class VKSampler;
        friend class VKDescriptorPool;
        friend class VKDescriptorSetLayout;
        friend class VKDescriptorSet;
        friend class VKShaderModule;
        friend class VKPipelineLayout;
        friend class VKPipelineCache;
        friend class VKGraphicsPipeline;
        friend class VKComputePipeline;
        vk::Device& GetHandle() { return m_Device; }
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
        ~VKDevice() override;
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
        Ref<CacaoSampler> CreateSampler(const SamplerCreateInfo& createInfo) override;
        std::shared_ptr<CacaoDescriptorSetLayout>
        CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) override;
        std::shared_ptr<CacaoDescriptorPool> CreateDescriptorPool(const DescriptorPoolCreateInfo& info) override;
        Ref<CacaoShaderModule> CreateShaderModule(const ShaderBlob& blob, const ShaderCreateInfo& info) override;
        Ref<CacaoPipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo& info) override;
        Ref<CacaoPipelineCache> CreatePipelineCache(const std::vector<uint8_t>& initialData) override;
        Ref<CacaoGraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info) override;
        Ref<CacaoComputePipeline> CreateComputePipeline(const ComputePipelineCreateInfo& info) override;
        Ref<CacaoSynchronization> CreateSynchronization(uint32_t maxFramesInFlight) override;
    };
}
#endif
