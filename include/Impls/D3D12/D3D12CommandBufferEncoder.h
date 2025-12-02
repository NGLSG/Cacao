#ifndef CACAO_D3D12COMMANDBUFFERENCODER_H
#define CACAO_D3D12COMMANDBUFFERENCODER_H
#ifdef WIN32
#include "CommandBufferEncoder.h"
#include "D3D12Common.h"
#include "d3d12.h"
#include "Barrier.h"

namespace Cacao
{
    class CACAO_API D3D12CommandBufferEncoder : public CommandBufferEncoder
    {
        ComPtr<ID3D12GraphicsCommandList10> m_commandList;

    public:
        void Free() override;
        void Reset() override;
        void ReturnToPool() override;
        void Begin(const CommandBufferBeginInfo& info) override;
        void End() override;
        void BeginRendering(const RenderingInfo& info) override;
        void EndRendering() override;
        void BindGraphicsPipeline(const Ref<GraphicsPipeline>& pipeline) override;
        void BindComputePipeline(const Ref<ComputePipeline>& pipeline) override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Rect2D& scissor) override;
        void BindVertexBuffer(uint32_t binding, const Ref<Buffer>& buffer, uint64_t offset) override;
        void BindIndexBuffer(const Ref<Buffer>& buffer, uint64_t offset, IndexType indexType) override;
        void BindDescriptorSets(const Ref<GraphicsPipeline>& pipeline, uint32_t firstSet,
                                std::span<const Ref<DescriptorSet>> descriptorSets) override;
        void PushConstants(const Ref<GraphicsPipeline>& pipeline, ShaderStage stageFlags, uint32_t offset,
                           uint32_t size, const void* data) override;
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance) override;
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        void BindComputeDescriptorSets(const Ref<ComputePipeline>& pipeline, uint32_t firstSet,
                                       std::span<const Ref<DescriptorSet>> descriptorSets) override;
        void ComputePushConstants(const Ref<ComputePipeline>& pipeline, ShaderStage stageFlags, uint32_t offset,
                                  uint32_t size, const void* data) override;
        void PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                             std::span<const CMemoryBarrier> globalBarriers,
                             std::span<const BufferBarrier> bufferBarriers,
                             std::span<const TextureBarrier> textureBarriers) override;
        void TransitionImage(const Ref<Texture>& texture, ImageTransition transition,
                             const ImageSubresourceRange& range) override;
        void TransitionBuffer(const Ref<Buffer>& buffer, BufferTransition transition, uint64_t offset,
                              uint64_t size) override;
        void MemoryBarrierFast(MemoryTransition transition) override;
        void ExecuteNative(const std::function<void(void* nativeCommandBuffer)>& func) override;
        void* GetNativeHandle() override;
        void CopyBufferToImage(const Ref<Buffer>& srcBuffer, const Ref<Texture>& dstImage, ImageLayout dstImageLayout,
                               std::span<const BufferImageCopy> regions) override;
        CommandBufferType GetCommandBufferType() const override;
    };
}
#endif
#endif
