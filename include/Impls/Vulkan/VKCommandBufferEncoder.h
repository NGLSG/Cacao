#ifndef CACAO_VKCOMMANDBUFFERENCODER_H
#define CACAO_VKCOMMANDBUFFERENCODER_H
#include "CacaoCommandBufferEncoder.h"
#include  <vulkan/vulkan.hpp>
namespace Cacao
{
    class VKDevice;
    class CacaoDevice;
    class CACAO_API VKCommandBufferEncoder : public CacaoCommandBufferEncoder
    {
    private:
        vk::CommandBuffer m_commandBuffer;
        Ref<VKDevice> m_device;
        friend class VKDevice;
        friend class VKQueue;
        vk::CommandBuffer& GetVulkanCommandBuffer() { return m_commandBuffer; }
        CommandBufferType m_type;
        vk::CommandBufferInheritanceRenderingInfo ConvertRenderingInfo(
            const RenderingInfo& info);
        vk::RenderingInfo ConvertRenderingInfoBegin(
            const RenderingInfo& info);
        std::vector<vk::RenderingAttachmentInfo> m_vkColorAttachments;
        vk::RenderingAttachmentInfo m_vkDepthAttachment;
        vk::RenderingAttachmentInfo m_vkStencilAttachment;
    public:
        static Ref<VKCommandBufferEncoder> Create(const Ref<CacaoDevice>& device, vk::CommandBuffer commandBuffer,
                                                  CommandBufferType type);
        VKCommandBufferEncoder(const Ref<CacaoDevice>& device, vk::CommandBuffer commandBuffer, CommandBufferType type);
        void Free() override;
        void Reset() override;
        void ReturnToPool() override;
        void Begin(const CommandBufferBeginInfo& info = CommandBufferBeginInfo()) override;
        void End() override;
        void BeginRendering(const RenderingInfo& info) override;
        void EndRendering() override;
        void BindGraphicsPipeline(const Ref<CacaoGraphicsPipeline>& pipeline) override;
        void BindComputePipeline(const Ref<CacaoComputePipeline>& pipeline) override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Rect2D& scissor) override;
        void BindVertexBuffer(uint32_t binding, const Ref<CacaoBuffer>& buffer, uint64_t offset) override;
        void BindIndexBuffer(const Ref<CacaoBuffer>& buffer, uint64_t offset, IndexType indexType) override;
        void BindDescriptorSets(const Ref<CacaoGraphicsPipeline>& pipeline, uint32_t firstSet,
                                const std::vector<Ref<CacaoDescriptorSet>>& descriptorSets) override;
        void PushConstants(const Ref<CacaoGraphicsPipeline>& pipeline, ShaderStage stageFlags,
                           uint32_t offset, uint32_t size, const void* data) override;
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance) override;
        void PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                             const std::vector<MemoryBarrier>& globalBarriers,
                             const std::vector<BufferBarrier>& bufferBarriers,
                             const std::vector<TextureBarrier>& textureBarriers) override;
        CommandBufferType GetCommandBufferType() const override
        {
            return m_type;
        }
    };
}
#endif 
