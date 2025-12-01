#ifndef CACAO_CACAOCOMMANDBUFFERENCODER_H
#define CACAO_CACAOCOMMANDBUFFERENCODER_H
#include "CacaoBarrier.h"
namespace Cacao
{
    class CacaoBuffer;
    class CacaoTexture;
    class CacaoGraphicsPipeline;
    class CacaoComputePipeline;
    class CacaoDescriptorSet;
    struct Viewport
    {
        float X;
        float Y;
        float Width;
        float Height;
        float MinDepth = 0.0f;
        float MaxDepth = 1.0f;
    };
    struct Rect2D
    {
        int32_t OffsetX;
        int32_t OffsetY;
        uint32_t Width;
        uint32_t Height;
    };
    struct ClearValue
    {
        union
        {
            float Color[4]; 
            struct
            {
                float Depth;
                uint32_t Stencil;
            } DepthStencil;
        };
        static ClearValue ColorFloat(float r, float g, float b, float a)
        {
            ClearValue v;
            v.Color[0] = r;
            v.Color[1] = g;
            v.Color[2] = b;
            v.Color[3] = a;
            return v;
        }
        static ClearValue DepthStencilValue(float d, uint32_t s)
        {
            ClearValue v;
            v.DepthStencil.Depth = d;
            v.DepthStencil.Stencil = s;
            return v;
        }
    };
    struct ClearDepthStencilValue
    {
        float Depth;
        uint32_t Stencil;
    };
    enum class IndexType
    {
        UInt16,
        UInt32
    };
    enum class AttachmentLoadOp
    {
        Load, 
        Clear, 
        DontCare 
    };
    enum class AttachmentStoreOp
    {
        Store, 
        DontCare 
    };
    struct RenderingAttachmentInfo
    {
        Ref<CacaoTexture> Texture;
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
        ClearValue ClearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        ClearDepthStencilValue ClearDepthStencil = {1.0f, 0};
    };
    struct RenderingInfo
    {
        Rect2D RenderArea;
        std::vector<RenderingAttachmentInfo> ColorAttachments;
        Ref<RenderingAttachmentInfo> DepthAttachment = nullptr;
        Ref<RenderingAttachmentInfo> StencilAttachment = nullptr;
        uint32_t LayerCount = 1;
    };
    struct CommandBufferInheritanceInfo
    {
        const RenderingInfo* pRenderingInfo = nullptr;
        bool OcclusionQueryEnable = false;
        bool PipelineStatistics = false;
    };
    struct CommandBufferBeginInfo
    {
        bool OneTimeSubmit = true;
        bool SimultaneousUse = false;
        const CommandBufferInheritanceInfo* InheritanceInfo = nullptr;
    };
    enum class CommandBufferType;
    class CACAO_API CacaoCommandBufferEncoder : public std::enable_shared_from_this<CacaoCommandBufferEncoder>
    {
    public:
        virtual void Free() =0;
        virtual void Reset() =0;
        virtual void ReturnToPool() =0;
        virtual void Begin(const CommandBufferBeginInfo& info = CommandBufferBeginInfo()) = 0;
        virtual void End() = 0;
        virtual void BeginRendering(const RenderingInfo& info) = 0;
        virtual void EndRendering() = 0;
        virtual void BindGraphicsPipeline(const Ref<CacaoGraphicsPipeline>& pipeline) = 0;
        virtual void BindComputePipeline(const Ref<CacaoComputePipeline>& pipeline) = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Rect2D& scissor) = 0;
        virtual void BindVertexBuffer(uint32_t binding, const Ref<CacaoBuffer>& buffer,
                                      uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(const Ref<CacaoBuffer>& buffer, uint64_t offset,
                                     IndexType indexType) = 0;
        virtual void BindDescriptorSets(
            const Ref<CacaoGraphicsPipeline>& pipeline,
            uint32_t firstSet,
            const std::vector<Ref<CacaoDescriptorSet>>& descriptorSets) = 0;
        virtual void PushConstants(
            const Ref<CacaoGraphicsPipeline>& pipeline,
            ShaderStage stageFlags,
            uint32_t offset,
            uint32_t size,
            const void* data) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                          uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                 uint32_t firstInstance) = 0;
        virtual void PipelineBarrier(
            PipelineStage srcStage,
            PipelineStage dstStage,
            const std::vector<MemoryBarrier>& globalBarriers,
            const std::vector<BufferBarrier>& bufferBarriers,
            const std::vector<TextureBarrier>& textureBarriers
        ) = 0;
        virtual void PipelineBarrier(
            PipelineStage srcStage,
            PipelineStage dstStage,
            const std::vector<TextureBarrier>& textureBarriers
        )
        {
            PipelineBarrier(srcStage, dstStage, {}, {}, textureBarriers);
        }
        virtual void PipelineBarrier(
            PipelineStage srcStage,
            PipelineStage dstStage,
            const std::vector<BufferBarrier>& bufferBarriers
        )
        {
            PipelineBarrier(srcStage, dstStage, {}, bufferBarriers, {});
        }
        virtual ~CacaoCommandBufferEncoder() = default;
        virtual CommandBufferType GetCommandBufferType() const =0;
    };
} 
#endif 
