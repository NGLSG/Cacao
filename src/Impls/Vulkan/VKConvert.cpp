#include "Impls/Vulkan/VKConvert.h"
#include "Buffer.h"
namespace Cacao
{
    vk::BufferUsageFlags Converter::Convert(BufferUsageFlags usage)
    {
        vk::BufferUsageFlags usageFlags;
        if (usage & BufferUsageFlags::VertexBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eVertexBuffer;
        if (usage & BufferUsageFlags::IndexBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eIndexBuffer;
        if (usage & BufferUsageFlags::UniformBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
        if (usage & BufferUsageFlags::StorageBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
        if (usage & BufferUsageFlags::TransferSrc)
            usageFlags |= vk::BufferUsageFlagBits::eTransferSrc;
        if (usage & BufferUsageFlags::TransferDst)
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        if (usage & BufferUsageFlags::IndirectBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
        if (usage & BufferUsageFlags::ShaderDeviceAddress)
            usageFlags |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        if (usage & BufferUsageFlags::AccelerationStructure)
            usageFlags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        return usageFlags;
    }
    VmaMemoryUsage Converter::Convert(BufferMemoryUsage usage)
    {
        switch (usage)
        {
        case BufferMemoryUsage::GpuOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferMemoryUsage::CpuOnly: return VMA_MEMORY_USAGE_CPU_ONLY;
        case BufferMemoryUsage::CpuToGpu: return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case BufferMemoryUsage::GpuToCpu: return VMA_MEMORY_USAGE_GPU_TO_CPU;
        default: return VMA_MEMORY_USAGE_GPU_ONLY;
        }
    }
    vk::Format Converter::Convert(Format format)
    {
        switch (format)
        {
        case Format::RGBA8_UNORM: return vk::Format::eR8G8B8A8Unorm;
        case Format::BGRA8_UNORM: return vk::Format::eB8G8R8A8Unorm;
        case Format::RGBA8_SRGB: return vk::Format::eR8G8B8A8Srgb;
        case Format::BGRA8_SRGB: return vk::Format::eB8G8R8A8Srgb;
        case Format::RGBA16_FLOAT: return vk::Format::eR16G16B16A16Sfloat;
        case Format::RGB10A2_UNORM: return vk::Format::eA2B10G10R10UnormPack32;
        case Format::RGBA32_FLOAT: return vk::Format::eR32G32B32A32Sfloat;
        case Format::R8_UNORM: return vk::Format::eR8Unorm;
        case Format::R16_FLOAT: return vk::Format::eR16Sfloat;
        case Format::D32F: return vk::Format::eD32Sfloat;
        case Format::D24S8: return vk::Format::eD24UnormS8Uint;
        case Format::UNDEFINED: return vk::Format::eUndefined;
        default: throw std::runtime_error("Unsupported texture format in VKConvert");
        }
    }
    vk::ShaderStageFlagBits Converter::ConvertShaderStageBits(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
        case ShaderStage::Geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderStage::TessellationControl: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderStage::TessellationEvaluation: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        default: throw std::runtime_error("Unknown ShaderStage");
        }
    }
    vk::ShaderStageFlags Converter::ConvertShaderStageFlags(ShaderStage stage)
    {
        vk::ShaderStageFlags flags;
        if (stage == ShaderStage::None) return flags;
        auto stageBits = static_cast<uint32_t>(stage);
        if (stageBits & static_cast<uint32_t>(ShaderStage::Vertex))
            flags |= vk::ShaderStageFlagBits::eVertex;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Fragment))
            flags |= vk::ShaderStageFlagBits::eFragment;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Compute))
            flags |= vk::ShaderStageFlagBits::eCompute;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Geometry))
            flags |= vk::ShaderStageFlagBits::eGeometry;
        if (stageBits & static_cast<uint32_t>(ShaderStage::TessellationControl))
            flags |= vk::ShaderStageFlagBits::eTessellationControl;
        if (stageBits & static_cast<uint32_t>(ShaderStage::TessellationEvaluation))
            flags |= vk::ShaderStageFlagBits::eTessellationEvaluation;
#ifdef VK_ENABLE_BETA_EXTENSIONS
        if (stageBits & static_cast<uint32_t>(ShaderStage::Mesh))
            flags |= vk::ShaderStageFlagBits::eMeshEXT;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Task))
            flags |= vk::ShaderStageFlagBits::eTaskEXT;
#endif
#ifdef VK_KHR_ray_tracing_pipeline
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayGen))
            flags |= vk::ShaderStageFlagBits::eRaygenKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayAnyHit))
            flags |= vk::ShaderStageFlagBits::eAnyHitKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayClosestHit))
            flags |= vk::ShaderStageFlagBits::eClosestHitKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayMiss))
            flags |= vk::ShaderStageFlagBits::eMissKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayIntersection))
            flags |= vk::ShaderStageFlagBits::eIntersectionKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Callable))
            flags |= vk::ShaderStageFlagBits::eCallableKHR;
#endif
        return flags;
    }
    vk::PrimitiveTopology Converter::Convert(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList: return vk::PrimitiveTopology::ePointList;
        case PrimitiveTopology::LineList: return vk::PrimitiveTopology::eLineList;
        case PrimitiveTopology::LineStrip: return vk::PrimitiveTopology::eLineStrip;
        case PrimitiveTopology::TriangleList: return vk::PrimitiveTopology::eTriangleList;
        case PrimitiveTopology::TriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
        case PrimitiveTopology::TriangleFan: return vk::PrimitiveTopology::eTriangleFan;
        case PrimitiveTopology::PatchList: return vk::PrimitiveTopology::ePatchList;
        default: throw std::runtime_error("Unknown PrimitiveTopology");
        }
    }
    vk::CullModeFlags Converter::Convert(CullMode cullMode)
    {
        switch (cullMode)
        {
        case CullMode::None: return vk::CullModeFlagBits::eNone;
        case CullMode::Front: return vk::CullModeFlagBits::eFront;
        case CullMode::Back: return vk::CullModeFlagBits::eBack;
        case CullMode::FrontAndBack: return vk::CullModeFlagBits::eFrontAndBack;
        default: throw std::runtime_error("Unknown CullMode");
        }
    }
    vk::FrontFace Converter::Convert(FrontFace frontFace)
    {
        switch (frontFace)
        {
        case FrontFace::CounterClockwise: return vk::FrontFace::eCounterClockwise;
        case FrontFace::Clockwise: return vk::FrontFace::eClockwise;
        default: throw std::runtime_error("Unknown FrontFace");
        }
    }
    vk::PolygonMode Converter::Convert(PolygonMode polygonMode)
    {
        switch (polygonMode)
        {
        case PolygonMode::Fill: return vk::PolygonMode::eFill;
        case PolygonMode::Line: return vk::PolygonMode::eLine;
        case PolygonMode::Point: return vk::PolygonMode::ePoint;
        default: throw std::runtime_error("Unknown PolygonMode");
        }
    }
    vk::LogicOp Converter::Convert(LogicOp logicOp)
    {
        switch (logicOp)
        {
        case LogicOp::Clear: return vk::LogicOp::eClear;
        case LogicOp::And: return vk::LogicOp::eAnd;
        case LogicOp::AndReverse: return vk::LogicOp::eAndReverse;
        case LogicOp::Copy: return vk::LogicOp::eCopy;
        case LogicOp::AndInverted: return vk::LogicOp::eAndInverted;
        case LogicOp::NoOp: return vk::LogicOp::eNoOp;
        case LogicOp::Xor: return vk::LogicOp::eXor;
        case LogicOp::Or: return vk::LogicOp::eOr;
        case LogicOp::Nor: return vk::LogicOp::eNor;
        case LogicOp::Equiv: return vk::LogicOp::eEquivalent;
        case LogicOp::Invert: return vk::LogicOp::eInvert;
        case LogicOp::OrReverse: return vk::LogicOp::eOrReverse;
        case LogicOp::CopyInverted: return vk::LogicOp::eCopyInverted;
        case LogicOp::OrInverted: return vk::LogicOp::eOrInverted;
        case LogicOp::Nand: return vk::LogicOp::eNand;
        case LogicOp::Set: return vk::LogicOp::eSet;
        default: throw std::runtime_error("Unknown LogicOp");
        }
    }
    vk::BlendFactor Converter::Convert(BlendFactor blendFactor)
    {
        switch (blendFactor)
        {
        case BlendFactor::Zero: return vk::BlendFactor::eZero;
        case BlendFactor::One: return vk::BlendFactor::eOne;
        case BlendFactor::SrcColor: return vk::BlendFactor::eSrcColor;
        case BlendFactor::OneMinusSrcColor: return vk::BlendFactor::eOneMinusSrcColor;
        case BlendFactor::DstColor: return vk::BlendFactor::eDstColor;
        case BlendFactor::OneMinusDstColor: return vk::BlendFactor::eOneMinusDstColor;
        case BlendFactor::SrcAlpha: return vk::BlendFactor::eSrcAlpha;
        case BlendFactor::OneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
        case BlendFactor::DstAlpha: return vk::BlendFactor::eDstAlpha;
        case BlendFactor::OneMinusDstAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
        case BlendFactor::ConstantColor: return vk::BlendFactor::eConstantColor;
        case BlendFactor::OneMinusConstantColor: return vk::BlendFactor::eOneMinusConstantColor;
        case BlendFactor::SrcAlphaSaturate: return vk::BlendFactor::eSrcAlphaSaturate;
        default: throw std::runtime_error("Unknown BlendFactor");
        }
    }
    vk::BlendOp Converter::Convert(BlendOp blendOp)
    {
        switch (blendOp)
        {
        case BlendOp::Add: return vk::BlendOp::eAdd;
        case BlendOp::Subtract: return vk::BlendOp::eSubtract;
        case BlendOp::ReverseSubtract: return vk::BlendOp::eReverseSubtract;
        case BlendOp::Min: return vk::BlendOp::eMin;
        case BlendOp::Max: return vk::BlendOp::eMax;
        default: throw std::runtime_error("Unknown BlendOp");
        }
    }
    vk::ColorComponentFlags Converter::Convert(ColorComponentFlags flags)
    {
        vk::ColorComponentFlags vkFlags;
        if (flags & ColorComponentFlags::R) vkFlags |= vk::ColorComponentFlagBits::eR;
        if (flags & ColorComponentFlags::G) vkFlags |= vk::ColorComponentFlagBits::eG;
        if (flags & ColorComponentFlags::B) vkFlags |= vk::ColorComponentFlagBits::eB;
        if (flags & ColorComponentFlags::A) vkFlags |= vk::ColorComponentFlagBits::eA;
        return vkFlags;
    }
    vk::CompareOp Converter::Convert(CompareOp compareOp)
    {
        switch (compareOp)
        {
        case CompareOp::Never: return vk::CompareOp::eNever;
        case CompareOp::Less: return vk::CompareOp::eLess;
        case CompareOp::Equal: return vk::CompareOp::eEqual;
        case CompareOp::LessOrEqual: return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater: return vk::CompareOp::eGreater;
        case CompareOp::NotEqual: return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::Always: return vk::CompareOp::eAlways;
        default: throw std::runtime_error("Unknown CompareOp");
        }
    }
    vk::StencilOp Converter::Convert(StencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case StencilOp::Keep: return vk::StencilOp::eKeep;
        case StencilOp::Zero: return vk::StencilOp::eZero;
        case StencilOp::Replace: return vk::StencilOp::eReplace;
        case StencilOp::IncrementAndClamp: return vk::StencilOp::eIncrementAndClamp;
        case StencilOp::DecrementAndClamp: return vk::StencilOp::eDecrementAndClamp;
        case StencilOp::Invert: return vk::StencilOp::eInvert;
        case StencilOp::IncrementWrap: return vk::StencilOp::eIncrementAndWrap;
        case StencilOp::DecrementWrap: return vk::StencilOp::eDecrementAndWrap;
        default: throw std::runtime_error("Unknown StencilOp");
        }
    }
    vk::SampleCountFlagBits Converter::ConvertSampleCount(uint32_t sampleCount)
    {
        switch (sampleCount)
        {
        case 1: return vk::SampleCountFlagBits::e1;
        case 2: return vk::SampleCountFlagBits::e2;
        case 4: return vk::SampleCountFlagBits::e4;
        case 8: return vk::SampleCountFlagBits::e8;
        case 16: return vk::SampleCountFlagBits::e16;
        case 32: return vk::SampleCountFlagBits::e32;
        case 64: return vk::SampleCountFlagBits::e64;
        default: throw std::runtime_error("Unsupported sample count");
        }
    }
    vk::PipelineStageFlags Converter::Convert(PipelineStage flags)
    {
        vk::PipelineStageFlags vkFlags;
        if (flags == PipelineStage::None) return vk::PipelineStageFlagBits::eTopOfPipe;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::TopOfPipe))
            vkFlags |= vk::PipelineStageFlagBits::eTopOfPipe;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::DrawIndirect))
            vkFlags |= vk::PipelineStageFlagBits::eDrawIndirect;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::VertexInput))
            vkFlags |= vk::PipelineStageFlagBits::eVertexInput;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::VertexShader))
            vkFlags |= vk::PipelineStageFlagBits::eVertexShader;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::GeometryShader))
            vkFlags |= vk::PipelineStageFlagBits::eGeometryShader;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::FragmentShader))
            vkFlags |= vk::PipelineStageFlagBits::eFragmentShader;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::EarlyFragmentTests))
            vkFlags |= vk::PipelineStageFlagBits::eEarlyFragmentTests;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::LateFragmentTests))
            vkFlags |= vk::PipelineStageFlagBits::eLateFragmentTests;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::ColorAttachmentOutput))
            vkFlags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::ComputeShader))
            vkFlags |= vk::PipelineStageFlagBits::eComputeShader;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::Transfer))
            vkFlags |= vk::PipelineStageFlagBits::eTransfer;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::BottomOfPipe))
            vkFlags |= vk::PipelineStageFlagBits::eBottomOfPipe;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::Host))
            vkFlags |= vk::PipelineStageFlagBits::eHost;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::AllGraphics))
            vkFlags |= vk::PipelineStageFlagBits::eAllGraphics;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStage::AllCommands))
            vkFlags |= vk::PipelineStageFlagBits::eAllCommands;
        return vkFlags;
    }
    vk::AccessFlags Converter::Convert(AccessFlags flags)
    {
        vk::AccessFlags vkFlags;
        if (flags == AccessFlags::None) return vkFlags;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::IndirectCommandRead))
            vkFlags |= vk::AccessFlagBits::eIndirectCommandRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::IndexRead))
            vkFlags |= vk::AccessFlagBits::eIndexRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::VertexAttributeRead))
            vkFlags |= vk::AccessFlagBits::eVertexAttributeRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::UniformRead))
            vkFlags |= vk::AccessFlagBits::eUniformRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::InputAttachmentRead))
            vkFlags |= vk::AccessFlagBits::eInputAttachmentRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderRead))
            vkFlags |= vk::AccessFlagBits::eShaderRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderWrite))
            vkFlags |= vk::AccessFlagBits::eShaderWrite;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ColorAttachmentRead))
            vkFlags |= vk::AccessFlagBits::eColorAttachmentRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ColorAttachmentWrite))
            vkFlags |= vk::AccessFlagBits::eColorAttachmentWrite;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentRead))
            vkFlags |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentWrite))
            vkFlags |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferRead))
            vkFlags |= vk::AccessFlagBits::eTransferRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferWrite))
            vkFlags |= vk::AccessFlagBits::eTransferWrite;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::HostRead))
            vkFlags |= vk::AccessFlagBits::eHostRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::HostWrite))
            vkFlags |= vk::AccessFlagBits::eHostWrite;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::MemoryRead))
            vkFlags |= vk::AccessFlagBits::eMemoryRead;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::MemoryWrite))
            vkFlags |= vk::AccessFlagBits::eMemoryWrite;
        return vkFlags;
    }
    vk::ImageLayout Converter::Convert(ImageLayout layout)
    {
        switch (layout)
        {
        case ImageLayout::Undefined: return vk::ImageLayout::eUndefined;
        case ImageLayout::General: return vk::ImageLayout::eGeneral;
        case ImageLayout::ColorAttachment: return vk::ImageLayout::eColorAttachmentOptimal;
        case ImageLayout::DepthStencilAttachment: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case ImageLayout::DepthStencilReadOnly: return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        case ImageLayout::ShaderReadOnly: return vk::ImageLayout::eShaderReadOnlyOptimal;
        case ImageLayout::TransferSrc: return vk::ImageLayout::eTransferSrcOptimal;
        case ImageLayout::TransferDst: return vk::ImageLayout::eTransferDstOptimal;
        case ImageLayout::Present: return vk::ImageLayout::ePresentSrcKHR;
        case ImageLayout::Preinitialized: return vk::ImageLayout::ePreinitialized;
        default: return vk::ImageLayout::eUndefined;
        }
    }
    vk::ImageAspectFlags Converter::Convert(ImageAspectFlags flags)
    {
        vk::ImageAspectFlags vkFlags;
        if (flags == ImageAspectFlags::None) return vkFlags;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Color))
            vkFlags |= vk::ImageAspectFlagBits::eColor;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Depth))
            vkFlags |= vk::ImageAspectFlagBits::eDepth;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Stencil))
            vkFlags |= vk::ImageAspectFlagBits::eStencil;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Metadata))
            vkFlags |= vk::ImageAspectFlagBits::eMetadata;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Plane0))
            vkFlags |= vk::ImageAspectFlagBits::ePlane0;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Plane1))
            vkFlags |= vk::ImageAspectFlagBits::ePlane1;
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(ImageAspectFlags::Plane2))
            vkFlags |= vk::ImageAspectFlagBits::ePlane2;
        return vkFlags;
    }
    vk::Filter Converter::Convert(Filter filter)
    {
        switch (filter)
        {
        case Filter::Nearest: return vk::Filter::eNearest;
        case Filter::Linear: return vk::Filter::eLinear;
        default: return vk::Filter::eLinear;
        }
    }
    vk::SamplerAddressMode Converter::Convert(SamplerAddressMode addressMode)
    {
        switch (addressMode)
        {
        case SamplerAddressMode::Repeat: return vk::SamplerAddressMode::eRepeat;
        case SamplerAddressMode::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
        case SamplerAddressMode::ClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
        case SamplerAddressMode::ClampToBorder: return vk::SamplerAddressMode::eClampToBorder;
        case SamplerAddressMode::MirrorClampToEdge: return vk::SamplerAddressMode::eMirrorClampToEdge;
        default: return vk::SamplerAddressMode::eRepeat;
        }
    }
    vk::SamplerMipmapMode Converter::Convert(SamplerMipmapMode mipmapMode)
    {
        switch (mipmapMode)
        {
        case SamplerMipmapMode::Nearest: return vk::SamplerMipmapMode::eNearest;
        case SamplerMipmapMode::Linear: return vk::SamplerMipmapMode::eLinear;
        default: return vk::SamplerMipmapMode::eLinear;
        }
    }
    vk::BorderColor Converter::Convert(BorderColor borderColor)
    {
        switch (borderColor)
        {
        case BorderColor::FloatTransparentBlack: return vk::BorderColor::eFloatTransparentBlack;
        case BorderColor::IntTransparentBlack: return vk::BorderColor::eIntTransparentBlack;
        case BorderColor::FloatOpaqueBlack: return vk::BorderColor::eFloatOpaqueBlack;
        case BorderColor::IntOpaqueBlack: return vk::BorderColor::eIntOpaqueBlack;
        case BorderColor::FloatOpaqueWhite: return vk::BorderColor::eFloatOpaqueWhite;
        case BorderColor::IntOpaqueWhite: return vk::BorderColor::eIntOpaqueWhite;
        default: return vk::BorderColor::eFloatOpaqueBlack;
        }
    }
    vk::DescriptorType Converter::Convert(DescriptorType type)
    {
        switch (type)
        {
        case DescriptorType::Sampler: return vk::DescriptorType::eSampler;
        case DescriptorType::CombinedImageSampler: return vk::DescriptorType::eCombinedImageSampler;
        case DescriptorType::SampledImage: return vk::DescriptorType::eSampledImage;
        case DescriptorType::StorageImage: return vk::DescriptorType::eStorageImage;
        case DescriptorType::UniformBuffer: return vk::DescriptorType::eUniformBuffer;
        case DescriptorType::StorageBuffer: return vk::DescriptorType::eStorageBuffer;
        case DescriptorType::UniformBufferDynamic: return vk::DescriptorType::eUniformBufferDynamic;
        case DescriptorType::StorageBufferDynamic: return vk::DescriptorType::eStorageBufferDynamic;
        case DescriptorType::InputAttachment: return vk::DescriptorType::eInputAttachment;
        case DescriptorType::AccelerationStructure: return vk::DescriptorType::eAccelerationStructureKHR;
        default: return vk::DescriptorType::eSampler;
        }
    }
    vk::IndexType Converter::Convert(IndexType indexType)
    {
        switch (indexType)
        {
        case IndexType::UInt16: return vk::IndexType::eUint16;
        case IndexType::UInt32: return vk::IndexType::eUint32;
        default: throw std::runtime_error("Unsupported index type");
        }
    }
}
