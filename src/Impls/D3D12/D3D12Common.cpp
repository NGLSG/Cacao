#ifdef WIN32
#include "Impls/D3D12/D3D12Common.h"
#include <stdexcept>

namespace Cacao
{
    D3D12_FEATURE D3D12Converter::Convert(DeviceFeature feature)
    {
        auto it = m_featureMap.find(feature);
        if (it != m_featureMap.end())
            return it->second;
        return D3D12_FEATURE_D3D12_OPTIONS;
    }

    D3D12_RESOURCE_FLAGS D3D12Converter::ConvertBufferUsage(BufferUsageFlags usage)
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        // 存储缓冲区需要 UAV 访问
        if (usage & BufferUsageFlags::StorageBuffer)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        return flags;
    }

    D3D12_HEAP_TYPE D3D12Converter::Convert(BufferMemoryUsage usage)
    {
        switch (usage)
        {
        case BufferMemoryUsage::GpuOnly:
            return D3D12_HEAP_TYPE_DEFAULT;
        case BufferMemoryUsage::CpuOnly:
            return D3D12_HEAP_TYPE_READBACK;
        case BufferMemoryUsage::CpuToGpu:
            return D3D12_HEAP_TYPE_UPLOAD;
        case BufferMemoryUsage::GpuToCpu:
            return D3D12_HEAP_TYPE_READBACK;
        default:
            return D3D12_HEAP_TYPE_DEFAULT;
        }
    }

    DXGI_FORMAT D3D12Converter::Convert(Format format)
    {
        switch (format)
        {
        // R8 格式
        case Format::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
        case Format::R8_SNORM: return DXGI_FORMAT_R8_SNORM;
        case Format::R8_UINT: return DXGI_FORMAT_R8_UINT;
        case Format::R8_SINT: return DXGI_FORMAT_R8_SINT;

        // RG8 格式
        case Format::RG8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
        case Format::RG8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
        case Format::RG8_UINT: return DXGI_FORMAT_R8G8_UINT;
        case Format::RG8_SINT: return DXGI_FORMAT_R8G8_SINT;

        // RGBA8 格式
        case Format::RGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case Format::RGBA8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
        case Format::RGBA8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
        case Format::RGBA8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        // BGRA8 格式
        case Format::BGRA8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        // R16 格式
        case Format::R16_UNORM: return DXGI_FORMAT_R16_UNORM;
        case Format::R16_SNORM: return DXGI_FORMAT_R16_SNORM;
        case Format::R16_UINT: return DXGI_FORMAT_R16_UINT;
        case Format::R16_SINT: return DXGI_FORMAT_R16_SINT;
        case Format::R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;

        // RG16 格式
        case Format::RG16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
        case Format::RG16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
        case Format::RG16_UINT: return DXGI_FORMAT_R16G16_UINT;
        case Format::RG16_SINT: return DXGI_FORMAT_R16G16_SINT;
        case Format::RG16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;

        // RGBA16 格式
        case Format::RGBA16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case Format::RGBA16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
        case Format::RGBA16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
        case Format::RGBA16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;

        // R32 格式
        case Format::R32_UINT: return DXGI_FORMAT_R32_UINT;
        case Format::R32_SINT: return DXGI_FORMAT_R32_SINT;
        case Format::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;

        // RG32 格式
        case Format::RG32_UINT: return DXGI_FORMAT_R32G32_UINT;
        case Format::RG32_SINT: return DXGI_FORMAT_R32G32_SINT;
        case Format::RG32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;

        // RGB32 格式
        case Format::RGB32_UINT: return DXGI_FORMAT_R32G32B32_UINT;
        case Format::RGB32_SINT: return DXGI_FORMAT_R32G32B32_SINT;
        case Format::RGB32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;

        // RGBA32 格式
        case Format::RGBA32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
        case Format::RGBA32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;

        // 打包格式
        case Format::RGB10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case Format::RGB10A2_UINT: return DXGI_FORMAT_R10G10B10A2_UINT;
        case Format::RG11B10_FLOAT: return DXGI_FORMAT_R11G11B10_FLOAT;
        case Format::RGB9E5_FLOAT: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;

        // 深度模板格式
        case Format::D16_UNORM: return DXGI_FORMAT_D16_UNORM;
        case Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
        case Format::D32_FLOAT_S8_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case Format::S8_UINT: return DXGI_FORMAT_R8_UINT; // D3D12 没有独立的 S8，使用 R8 替代

        // BC 压缩格式
        case Format::BC1_RGB_UNORM: return DXGI_FORMAT_BC1_UNORM;
        case Format::BC1_RGB_SRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case Format::BC1_RGBA_UNORM: return DXGI_FORMAT_BC1_UNORM;
        case Format::BC1_RGBA_SRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case Format::BC2_UNORM: return DXGI_FORMAT_BC2_UNORM;
        case Format::BC2_SRGB: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case Format::BC3_UNORM: return DXGI_FORMAT_BC3_UNORM;
        case Format::BC3_SRGB: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case Format::BC4_UNORM: return DXGI_FORMAT_BC4_UNORM;
        case Format::BC4_SNORM: return DXGI_FORMAT_BC4_SNORM;
        case Format::BC5_UNORM: return DXGI_FORMAT_BC5_UNORM;
        case Format::BC5_SNORM: return DXGI_FORMAT_BC5_SNORM;
        case Format::BC6H_UFLOAT: return DXGI_FORMAT_BC6H_UF16;
        case Format::BC6H_SFLOAT: return DXGI_FORMAT_BC6H_SF16;
        case Format::BC7_UNORM: return DXGI_FORMAT_BC7_UNORM;
        case Format::BC7_SRGB: return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            throw std::runtime_error("Unsupported texture format in D3D12Converter");
        }
    }

    D3D12_SHADER_VISIBILITY D3D12Converter::ConvertShaderVisibility(ShaderStage stage)
    {
        // 如果是单个阶段，返回对应的可见性
        switch (stage)
        {
        case ShaderStage::Vertex:
            return D3D12_SHADER_VISIBILITY_VERTEX;
        case ShaderStage::Fragment:
            return D3D12_SHADER_VISIBILITY_PIXEL;
        case ShaderStage::Geometry:
            return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case ShaderStage::TessellationControl:
            return D3D12_SHADER_VISIBILITY_HULL;
        case ShaderStage::TessellationEvaluation:
            return D3D12_SHADER_VISIBILITY_DOMAIN;
        case ShaderStage::Mesh:
            return D3D12_SHADER_VISIBILITY_MESH;
        case ShaderStage::Task:
            return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
        default:
            // 多个阶段或未知阶段使用 ALL
            return D3D12_SHADER_VISIBILITY_ALL;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY D3D12Converter::Convert(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case PrimitiveTopology::TriangleFan:
            // D3D12 不支持 TriangleFan，回退到 TriangleList
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::PatchList:
            // 默认使用 3 个控制点的 patch
            return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:
            throw std::runtime_error("Unknown PrimitiveTopology");
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12Converter::ConvertTopologyType(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::LineList:
        case PrimitiveTopology::LineStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::TriangleList:
        case PrimitiveTopology::TriangleStrip:
        case PrimitiveTopology::TriangleFan:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PrimitiveTopology::PatchList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    }

    D3D12_CULL_MODE D3D12Converter::Convert(CullMode cullMode)
    {
        switch (cullMode)
        {
        case CullMode::None:
            return D3D12_CULL_MODE_NONE;
        case CullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case CullMode::Back:
            return D3D12_CULL_MODE_BACK;
        case CullMode::FrontAndBack:
            // D3D12 不支持同时剔除正面和背面，返回 NONE
            return D3D12_CULL_MODE_NONE;
        default:
            throw std::runtime_error("Unknown CullMode");
        }
    }

    D3D12_FILL_MODE D3D12Converter::Convert(PolygonMode polygonMode)
    {
        switch (polygonMode)
        {
        case PolygonMode::Fill:
            return D3D12_FILL_MODE_SOLID;
        case PolygonMode::Line:
            return D3D12_FILL_MODE_WIREFRAME;
        case PolygonMode::Point:
            // D3D12 不直接支持点模式，使用线框模式
            return D3D12_FILL_MODE_WIREFRAME;
        default:
            throw std::runtime_error("Unknown PolygonMode");
        }
    }

    BOOL D3D12Converter::ConvertFrontFace(FrontFace frontFace)
    {
        // D3D12 中 FrontCounterClockwise 为 TRUE 表示逆时针为正面
        switch (frontFace)
        {
        case FrontFace::CounterClockwise:
            return TRUE;
        case FrontFace::Clockwise:
            return FALSE;
        default:
            throw std::runtime_error("Unknown FrontFace");
        }
    }

    D3D12_LOGIC_OP D3D12Converter::Convert(LogicOp logicOp)
    {
        switch (logicOp)
        {
        case LogicOp::Clear: return D3D12_LOGIC_OP_CLEAR;
        case LogicOp::And: return D3D12_LOGIC_OP_AND;
        case LogicOp::AndReverse: return D3D12_LOGIC_OP_AND_REVERSE;
        case LogicOp::Copy: return D3D12_LOGIC_OP_COPY;
        case LogicOp::AndInverted: return D3D12_LOGIC_OP_AND_INVERTED;
        case LogicOp::NoOp: return D3D12_LOGIC_OP_NOOP;
        case LogicOp::Xor: return D3D12_LOGIC_OP_XOR;
        case LogicOp::Or: return D3D12_LOGIC_OP_OR;
        case LogicOp::Nor: return D3D12_LOGIC_OP_NOR;
        case LogicOp::Equiv: return D3D12_LOGIC_OP_EQUIV;
        case LogicOp::Invert: return D3D12_LOGIC_OP_INVERT;
        case LogicOp::OrReverse: return D3D12_LOGIC_OP_OR_REVERSE;
        case LogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
        case LogicOp::OrInverted: return D3D12_LOGIC_OP_OR_INVERTED;
        case LogicOp::Nand: return D3D12_LOGIC_OP_NAND;
        case LogicOp::Set: return D3D12_LOGIC_OP_SET;
        default:
            throw std::runtime_error("Unknown LogicOp");
        }
    }

    D3D12_BLEND D3D12Converter::Convert(BlendFactor blendFactor)
    {
        switch (blendFactor)
        {
        case BlendFactor::Zero: return D3D12_BLEND_ZERO;
        case BlendFactor::One: return D3D12_BLEND_ONE;
        case BlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactor::DstColor: return D3D12_BLEND_DEST_COLOR;
        case BlendFactor::OneMinusDstColor: return D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
        case BlendFactor::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
        case BlendFactor::ConstantColor: return D3D12_BLEND_BLEND_FACTOR;
        case BlendFactor::OneMinusConstantColor: return D3D12_BLEND_INV_BLEND_FACTOR;
        case BlendFactor::SrcAlphaSaturate: return D3D12_BLEND_SRC_ALPHA_SAT;
        default:
            throw std::runtime_error("Unknown BlendFactor");
        }
    }

    D3D12_BLEND_OP D3D12Converter::Convert(BlendOp blendOp)
    {
        switch (blendOp)
        {
        case BlendOp::Add: return D3D12_BLEND_OP_ADD;
        case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendOp::Min: return D3D12_BLEND_OP_MIN;
        case BlendOp::Max: return D3D12_BLEND_OP_MAX;
        default:
            throw std::runtime_error("Unknown BlendOp");
        }
    }

    UINT8 D3D12Converter::Convert(ColorComponentFlags flags)
    {
        UINT8 result = 0;
        if (flags & ColorComponentFlags::R) result |= D3D12_COLOR_WRITE_ENABLE_RED;
        if (flags & ColorComponentFlags::G) result |= D3D12_COLOR_WRITE_ENABLE_GREEN;
        if (flags & ColorComponentFlags::B) result |= D3D12_COLOR_WRITE_ENABLE_BLUE;
        if (flags & ColorComponentFlags::A) result |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
        return result;
    }

    D3D12_COMPARISON_FUNC D3D12Converter::Convert(CompareOp compareOp)
    {
        switch (compareOp)
        {
        case CompareOp::Never: return D3D12_COMPARISON_FUNC_NEVER;
        case CompareOp::Less: return D3D12_COMPARISON_FUNC_LESS;
        case CompareOp::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareOp::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareOp::Greater: return D3D12_COMPARISON_FUNC_GREATER;
        case CompareOp::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareOp::GreaterOrEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareOp::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            throw std::runtime_error("Unknown CompareOp");
        }
    }

    D3D12_STENCIL_OP D3D12Converter::Convert(StencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case StencilOp::Keep: return D3D12_STENCIL_OP_KEEP;
        case StencilOp::Zero: return D3D12_STENCIL_OP_ZERO;
        case StencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
        case StencilOp::IncrementAndClamp: return D3D12_STENCIL_OP_INCR_SAT;
        case StencilOp::DecrementAndClamp: return D3D12_STENCIL_OP_DECR_SAT;
        case StencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
        case StencilOp::IncrementWrap: return D3D12_STENCIL_OP_INCR;
        case StencilOp::DecrementWrap: return D3D12_STENCIL_OP_DECR;
        default:
            throw std::runtime_error("Unknown StencilOp");
        }
    }

    D3D12_RESOURCE_STATES D3D12Converter::Convert(ImageLayout layout)
    {
        switch (layout)
        {
        case ImageLayout::Undefined:
            return D3D12_RESOURCE_STATE_COMMON;
        case ImageLayout::General:
            return D3D12_RESOURCE_STATE_COMMON;
        case ImageLayout::ColorAttachment:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case ImageLayout::DepthStencilAttachment:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case ImageLayout::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case ImageLayout::ShaderReadOnly:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ImageLayout::TransferSrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case ImageLayout::TransferDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case ImageLayout::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        case ImageLayout::Preinitialized:
            return D3D12_RESOURCE_STATE_COMMON;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    D3D12_FILTER D3D12Converter::ConvertFilter(Filter minFilter, Filter magFilter,
                                               SamplerMipmapMode mipmapMode, bool anisotropyEnable)
    {
        if (anisotropyEnable)
        {
            return D3D12_FILTER_ANISOTROPIC;
        }

        // D3D12 过滤器编码规则:
        // Bit 0: Mip filter (0 = Point, 1 = Linear)
        // Bit 2-3: Mag filter (0 = Point, 1 = Linear)
        // Bit 4-5: Min filter (0 = Point, 1 = Linear)

        int mipValue = (mipmapMode == SamplerMipmapMode::Nearest) ? 0 : 1;
        int magValue = (magFilter == Filter::Nearest) ? 0 : 1;
        int minValue = (minFilter == Filter::Nearest) ? 0 : 1;

        // 计算过滤器值
        int filterValue = mipValue | (magValue << 2) | (minValue << 4);

        return static_cast<D3D12_FILTER>(filterValue);
    }

    D3D12_TEXTURE_ADDRESS_MODE D3D12Converter::Convert(SamplerAddressMode addressMode)
    {
        switch (addressMode)
        {
        case SamplerAddressMode::Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case SamplerAddressMode::MirroredRepeat:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case SamplerAddressMode::ClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case SamplerAddressMode::ClampToBorder:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case SamplerAddressMode::MirrorClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    D3D12_STATIC_BORDER_COLOR D3D12Converter::ConvertStaticBorderColor(BorderColor borderColor)
    {
        switch (borderColor)
        {
        case BorderColor::FloatTransparentBlack:
        case BorderColor::IntTransparentBlack:
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        case BorderColor::FloatOpaqueBlack:
        case BorderColor::IntOpaqueBlack:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        case BorderColor::FloatOpaqueWhite:
        case BorderColor::IntOpaqueWhite:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        default:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        }
    }

    // 在 Impls/D3D12/D3D12Common.cpp 中找到 Convert(DescriptorType) 函数并修改：

    D3D12_DESCRIPTOR_RANGE_TYPE D3D12Converter::Convert(DescriptorType type)
    {
        switch (type)
        {
        case DescriptorType::Sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

        case DescriptorType::CombinedImageSampler:
        case DescriptorType::SampledImage:
        case DescriptorType::InputAttachment:
        case DescriptorType::AccelerationStructure:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

            // ================== 关键修复 ==================
            // 将 StorageBuffer 映射为 SRV 以匹配 HLSL 的 StructuredBuffer<T> (register(tX))
        case DescriptorType::StorageBuffer:
        case DescriptorType::StorageBufferDynamic:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            // ============================================

        case DescriptorType::StorageImage:
            return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        case DescriptorType::UniformBuffer:
        case DescriptorType::UniformBufferDynamic:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

        default:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        }
    }

    DXGI_FORMAT D3D12Converter::Convert(IndexType indexType)
    {
        switch (indexType)
        {
        case IndexType::UInt16:
            return DXGI_FORMAT_R16_UINT;
        case IndexType::UInt32:
            return DXGI_FORMAT_R32_UINT;
        default:
            throw std::runtime_error("Unsupported index type");
        }
    }

    UINT D3D12Converter::ConvertSampleCount(uint32_t sampleCount)
    {
        // D3D12 直接使用数值作为采样数
        switch (sampleCount)
        {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
        case 64:
            return sampleCount;
        default:
            throw std::runtime_error("Unsupported sample count");
        }
    }
}
#endif
