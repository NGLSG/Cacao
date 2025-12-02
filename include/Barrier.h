#ifndef CACAO_CACAOBARRIER_H
#define CACAO_CACAOBARRIER_H
#include <vector>
namespace Cacao
{
    class Buffer;
    class Texture;
    enum class PipelineStage : uint32_t
    {
        None = 0,
        TopOfPipe = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShader = 1 << 3,
        HullShader = 1 << 4,
        DomainShader = 1 << 5,
        GeometryShader = 1 << 6,
        FragmentShader = 1 << 7,
        EarlyFragmentTests = 1 << 8,
        LateFragmentTests = 1 << 9,
        ColorAttachmentOutput = 1 << 10,
        ComputeShader = 1 << 11,
        Transfer = 1 << 12,
        BottomOfPipe = 1 << 13,
        Host = 1 << 14,
        AllGraphics = 1 << 15,
        AllCommands = 1 << 16,
    };
    inline PipelineStage operator|(PipelineStage a, PipelineStage b)
    {
        return static_cast<PipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    enum class AccessFlags : uint32_t
    {
        None = 0,
        IndirectCommandRead = 1 << 0,
        IndexRead = 1 << 1,
        VertexAttributeRead = 1 << 2,
        UniformRead = 1 << 3,
        InputAttachmentRead = 1 << 4,
        ShaderRead = 1 << 5,
        ShaderWrite = 1 << 6,
        ColorAttachmentRead = 1 << 7,
        ColorAttachmentWrite = 1 << 8,
        DepthStencilAttachmentRead = 1 << 9,
        DepthStencilAttachmentWrite = 1 << 10,
        TransferRead = 1 << 11,
        TransferWrite = 1 << 12,
        HostRead = 1 << 13,
        HostWrite = 1 << 14,
        MemoryRead = 1 << 15,
        MemoryWrite = 1 << 16
    };
    inline AccessFlags operator|(AccessFlags a, AccessFlags b)
    {
        return static_cast<AccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    enum class ImageLayout
    {
        Undefined,
        General,
        ColorAttachment,
        DepthStencilAttachment,
        DepthStencilReadOnly,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present,
        Preinitialized
    };
    enum class ImageAspectFlags : uint32_t
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        Metadata = 1 << 3,
        Plane0 = 1 << 4,
        Plane1 = 1 << 5,
        Plane2 = 1 << 6
    };
    inline ImageAspectFlags operator|(ImageAspectFlags a, ImageAspectFlags b)
    {
        return static_cast<ImageAspectFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline bool operator&(ImageAspectFlags a, ImageAspectFlags b)
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }
    inline ImageAspectFlags& operator|=(ImageAspectFlags& a, ImageAspectFlags b)
    {
        a = a | b;
        return a;
    }
    struct ImageSubresourceRange
    {
        uint32_t BaseMipLevel = 0;
        uint32_t LevelCount = 1;
        uint32_t BaseArrayLayer = 0;
        uint32_t LayerCount = 1;
        ImageAspectFlags AspectMask = ImageAspectFlags::Color;
        static ImageSubresourceRange All() { return {0, UINT32_MAX, 0, UINT32_MAX}; }
        static ImageSubresourceRange Mip0() { return {0, 1, 0, UINT32_MAX}; }
    };
    struct MemoryBarrier
    {
        AccessFlags SrcAccess;
        AccessFlags DstAccess;
    };
    struct BufferBarrier
    {
        Ref<Buffer> Buffer;
        AccessFlags SrcAccess;
        AccessFlags DstAccess;
        uint64_t Offset = 0;
        uint64_t Size = UINT64_MAX;
        uint32_t SrcQueueFamilyIndex = UINT32_MAX;
        uint32_t DstQueueFamilyIndex = UINT32_MAX;
    };
    struct TextureBarrier
    {
        Ref<Texture> Texture;
        AccessFlags SrcAccess;
        AccessFlags DstAccess;
        ImageLayout OldLayout;
        ImageLayout NewLayout;
        ImageSubresourceRange SubresourceRange = ImageSubresourceRange::All();
        uint32_t SrcQueueFamilyIndex = UINT32_MAX;
        uint32_t DstQueueFamilyIndex = UINT32_MAX;
    };
}
#endif
