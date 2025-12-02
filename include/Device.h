#ifndef CACAO_CACAODEVICE_H
#define CACAO_CACAODEVICE_H
#include <complex.h>
#include "ShaderModule.h"
namespace Cacao
{
    class Synchronization;
}
namespace Cacao
{
    struct ComputePipelineCreateInfo;
    class ComputePipeline;
    struct GraphicsPipelineCreateInfo;
    class GraphicsPipeline;
    class PipelineCache;
    struct PipelineLayoutCreateInfo;
    class PipelineLayout;
    struct DescriptorPoolCreateInfo;
    class DescriptorPool;
    struct DescriptorSetLayoutCreateInfo;
    class DescriptorSetLayout;
    struct SamplerCreateInfo;
    class Sampler;
    struct BufferCreateInfo;
    class Buffer;
    struct TextureCreateInfo;
    class Texture;
    class CommandBufferEncoder;
    class Adapter;
    struct SwapchainCreateInfo;
    class Swapchain;
    class Queue;
    class Surface;
    enum class QueueType;
    enum class CacaoDeviceFeature : uint32_t
    {
        GeometryShader,
        TessellationShader,
        MultiDrawIndirect,
        FillModeNonSolid,
        WideLines,
        SamplerAnisotropy,
        TextureCompressionBC,
        TextureCompressionASTC,
        BindlessDescriptors,
        BufferDeviceAddress,
        MeshShader,
        TaskShader,
        RayTracingPipeline,
        RayTracingQuery,
        AccelerationStructure,
        VariableRateShading,
        ConditionalRendering,
        ShaderFloat64,
        ShaderInt16,
        SubgroupOperations
    };
    struct QueueRequest
    {
        QueueType Type;
        uint32_t Count = 1;
        float Priority = 1.0f;
    };
    struct DeviceCreateInfo
    {
        std::vector<CacaoDeviceFeature> EnabledFeatures;
        std::vector<QueueRequest> QueueRequests;
        Ref<Surface> CompatibleSurface = nullptr;
        void* Next = nullptr;
    };
    enum class CommandBufferType
    {
        Primary,
        Secondary
    };
    class CACAO_API Device : public std::enable_shared_from_this<Device>
    {
    public:
        virtual ~Device() = default;
        virtual void WaitIdle() const = 0;
        virtual Ref<Queue> GetQueue(QueueType type, uint32_t index = 0) = 0;
        virtual Ref<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) = 0;
        virtual std::vector<uint32_t> GetAllQueueFamilyIndices() const = 0;
        virtual Ref<Adapter> GetParentAdapter() const = 0;
        virtual Ref<CommandBufferEncoder> CreateCommandBufferEncoder(
            CommandBufferType type = CommandBufferType::Primary) = 0;
        virtual void ResetCommandPool() = 0;
        virtual void ReturnCommandBuffer(const Ref<CommandBufferEncoder>& encoder) = 0;
        virtual void FreeCommandBuffer(const Ref<CommandBufferEncoder>& encoder) = 0;
        virtual void ResetCommandBuffer(const Ref<CommandBufferEncoder>& encoder) = 0;
        virtual Ref<Texture> CreateTexture(const TextureCreateInfo& createInfo) = 0;
        virtual Ref<Buffer> CreateBuffer(const BufferCreateInfo& createInfo) = 0;
        virtual Ref<Sampler> CreateSampler(const SamplerCreateInfo& createInfo) = 0;
        virtual Ref<DescriptorSetLayout> CreateDescriptorSetLayout(
            const DescriptorSetLayoutCreateInfo& info) = 0;
        virtual Ref<DescriptorPool> CreateDescriptorPool(
            const DescriptorPoolCreateInfo& info) = 0;
        virtual Ref<ShaderModule> CreateShaderModule(const ShaderBlob& blob, const ShaderCreateInfo& info) =0;
        virtual Ref<PipelineLayout> CreatePipelineLayout(
            const PipelineLayoutCreateInfo& info) = 0;
        virtual Ref<PipelineCache> CreatePipelineCache(
            std::span<const uint8_t> initialData = {}) = 0;
        virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(
            const GraphicsPipelineCreateInfo& info) = 0;
        virtual Ref<ComputePipeline> CreateComputePipeline(
            const ComputePipelineCreateInfo& info) = 0;
        virtual Ref<Synchronization> CreateSynchronization(uint32_t maxFramesInFlight) = 0;
    };
}
#endif
