#ifndef CACAO_D3D12COMMON_H
#define CACAO_D3D12COMMON_H
#ifdef WIN32
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

#include "Core.h"
#include "Adapter.h"
#include "PipelineDefs.h"
#include "Barrier.h"
#include "Texture.h"
#include "Sampler.h"
#include "DescriptorSetLayout.h"
#include "CommandBufferEncoder.h"
#include "Buffer.h"

using Microsoft::WRL::ComPtr;

namespace Cacao
{
    // 快速转换命名空间 - 使用查找表进行高性能转换
    namespace D3D12FastConvert
    {
        // 资源状态查找表
        inline constexpr D3D12_RESOURCE_STATES kResourceStateLUT[] = {
            D3D12_RESOURCE_STATE_COMMON,                    // Undefined
            D3D12_RESOURCE_STATE_COMMON,                    // General
            D3D12_RESOURCE_STATE_RENDER_TARGET,             // ColorAttachment
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // DepthStencilAttachment
            D3D12_RESOURCE_STATE_DEPTH_READ,                // DepthStencilReadOnly
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,     // ShaderReadOnly
            D3D12_RESOURCE_STATE_COPY_SOURCE,               // TransferSrc
            D3D12_RESOURCE_STATE_COPY_DEST,                 // TransferDst
            D3D12_RESOURCE_STATE_PRESENT,                   // Present
            D3D12_RESOURCE_STATE_COMMON,                    // Preinitialized
        };

        inline D3D12_RESOURCE_STATES ResourceState(Cacao::ImageLayout layout) noexcept
        {
            return kResourceStateLUT[static_cast<uint32_t>(layout)];
        }
    }

    class D3D12Converter
    {
        inline static const std::unordered_map<DeviceFeature, D3D12_FEATURE> m_featureMap =
        {
            // 核心着色器阶段
            {DeviceFeature::GeometryShader, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::TessellationShader, D3D12_FEATURE_D3D12_OPTIONS},

            // 基础光栅化/绘制
            {DeviceFeature::MultiDrawIndirect, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::FillModeNonSolid, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::WideLines, D3D12_FEATURE_D3D12_OPTIONS}, // D3D12 不支持

            // 采样/纹理格式
            {DeviceFeature::SamplerAnisotropy, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::TextureCompressionBC, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::TextureCompressionASTC, D3D12_FEATURE_D3D12_OPTIONS8},

            // 描述符
            {DeviceFeature::BindlessDescriptors, D3D12_FEATURE_D3D12_OPTIONS},

            // GPU 寻址 (Win11+)
            {DeviceFeature::BufferDeviceAddress, D3D12_FEATURE_D3D12_OPTIONS12},

            // Mesh / Task 着色器
            {DeviceFeature::MeshShader, D3D12_FEATURE_D3D12_OPTIONS7},
            {DeviceFeature::TaskShader, D3D12_FEATURE_D3D12_OPTIONS7},

            // 光线追踪
            {DeviceFeature::RayTracingPipeline, D3D12_FEATURE_D3D12_OPTIONS5},
            {DeviceFeature::RayTracingQuery, D3D12_FEATURE_D3D12_OPTIONS5},
            {DeviceFeature::AccelerationStructure, D3D12_FEATURE_D3D12_OPTIONS5},

            // 可变速率着色
            {DeviceFeature::VariableRateShading, D3D12_FEATURE_D3D12_OPTIONS6},

            // 仅 Vulkan 支持（D3D12 不支持）
            {DeviceFeature::ConditionalRendering, D3D12_FEATURE_D3D12_OPTIONS},

            // 着色器数值格式
            {DeviceFeature::ShaderFloat64, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::ShaderInt16, D3D12_FEATURE_D3D12_OPTIONS},
            {DeviceFeature::SubgroupOperations, D3D12_FEATURE_D3D12_OPTIONS1},
        };

    public:
        // 特性转换
        static D3D12_FEATURE Convert(DeviceFeature feature);

        // 缓冲区相关
        static D3D12_RESOURCE_FLAGS ConvertBufferUsage(BufferUsageFlags usage);
        static D3D12_HEAP_TYPE Convert(BufferMemoryUsage usage);

        // 格式转换
        static DXGI_FORMAT Convert(Format format);

        // 着色器阶段
        static D3D12_SHADER_VISIBILITY ConvertShaderVisibility(ShaderStage stage);

        // 图元拓扑
        static D3D12_PRIMITIVE_TOPOLOGY Convert(PrimitiveTopology topology);
        static D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertTopologyType(PrimitiveTopology topology);

        // 光栅化状态
        static D3D12_CULL_MODE Convert(CullMode cullMode);
        static D3D12_FILL_MODE Convert(PolygonMode polygonMode);
        static BOOL ConvertFrontFace(FrontFace frontFace);

        // 混合状态
        static D3D12_LOGIC_OP Convert(LogicOp logicOp);
        static D3D12_BLEND Convert(BlendFactor blendFactor);
        static D3D12_BLEND_OP Convert(BlendOp blendOp);
        static UINT8 Convert(ColorComponentFlags flags);

        // 深度模板状态
        static D3D12_COMPARISON_FUNC Convert(CompareOp compareOp);
        static D3D12_STENCIL_OP Convert(StencilOp stencilOp);

        // 资源状态
        static D3D12_RESOURCE_STATES Convert(ImageLayout layout);

        // 采样器相关
        static D3D12_FILTER ConvertFilter(Filter minFilter, Filter magFilter, SamplerMipmapMode mipmapMode, bool anisotropyEnable);
        static D3D12_TEXTURE_ADDRESS_MODE Convert(SamplerAddressMode addressMode);
        static D3D12_STATIC_BORDER_COLOR ConvertStaticBorderColor(BorderColor borderColor);

        // 描述符类型
        static D3D12_DESCRIPTOR_RANGE_TYPE Convert(DescriptorType type);

        // 索引类型
        static DXGI_FORMAT Convert(IndexType indexType);

        // 多重采样
        static UINT ConvertSampleCount(uint32_t sampleCount);
    };
}
#endif
#endif