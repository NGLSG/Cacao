#ifndef CACAO_CACAODEVICE_H
#define CACAO_CACAODEVICE_H
#include <complex.h>
#include "CacaoShaderModule.h"
namespace Cacao
{
    class CacaoSynchronization;
}
namespace Cacao
{
    struct ComputePipelineCreateInfo;
    class CacaoComputePipeline;
    struct GraphicsPipelineCreateInfo;
    class CacaoGraphicsPipeline;
    class CacaoPipelineCache;
    struct PipelineLayoutCreateInfo;
    class CacaoPipelineLayout;
    struct DescriptorPoolCreateInfo;
    class CacaoDescriptorPool;
    struct DescriptorSetLayoutCreateInfo;
    class CacaoDescriptorSetLayout;
    struct SamplerCreateInfo;
    class CacaoSampler;
    struct BufferCreateInfo;
    class CacaoBuffer;
    struct TextureCreateInfo;
    class CacaoTexture;
    class CacaoCommandBufferEncoder;
    class CacaoAdapter;
    struct SwapchainCreateInfo;
    class CacaoSwapchain;
    class CacaoQueue;
    class CacaoSurface;
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
    struct CacaoQueueRequest
    {
        QueueType Type;
        uint32_t Count = 1;
        float Priority = 1.0f;
    };
    struct CacaoDeviceCreateInfo
    {
        std::vector<CacaoDeviceFeature> EnabledFeatures;
        std::vector<CacaoQueueRequest> QueueRequests;
        Ref<CacaoSurface> CompatibleSurface = nullptr;
        void* Next = nullptr;
    };
    enum class CommandBufferType
    {
        Primary,
        Secondary
    };
    class CACAO_API CacaoDevice : public std::enable_shared_from_this<CacaoDevice>
    {
    public:
        virtual ~CacaoDevice() = default;
        virtual void WaitIdle() const = 0;
        virtual Ref<CacaoQueue> GetQueue(QueueType type, uint32_t index = 0) = 0;
        virtual Ref<CacaoSwapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) = 0;
        virtual std::vector<uint32_t> GetAllQueueFamilyIndices() const = 0;
        virtual Ref<CacaoAdapter> GetParentAdapter() const = 0;
        virtual Ref<CacaoCommandBufferEncoder> CreateCommandBufferEncoder(
            CommandBufferType type = CommandBufferType::Primary) = 0;
        virtual void ResetCommandPool() = 0;
        virtual void ReturnCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) = 0;
        virtual void FreeCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) = 0;
        virtual void ResetCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder) = 0;
        virtual Ref<CacaoTexture> CreateTexture(const TextureCreateInfo& createInfo) = 0;
        virtual Ref<CacaoBuffer> CreateBuffer(const BufferCreateInfo& createInfo) = 0;
        virtual Ref<CacaoSampler> CreateSampler(const SamplerCreateInfo& createInfo) = 0;
        virtual Ref<CacaoDescriptorSetLayout> CreateDescriptorSetLayout(
            const DescriptorSetLayoutCreateInfo& info) = 0;
        virtual Ref<CacaoDescriptorPool> CreateDescriptorPool(
            const DescriptorPoolCreateInfo& info) = 0;
        virtual Ref<CacaoShaderModule> CreateShaderModule(const ShaderBlob& blob, const ShaderCreateInfo& info) =0;
        virtual Ref<CacaoPipelineLayout> CreatePipelineLayout(
            const PipelineLayoutCreateInfo& info) = 0;
        virtual Ref<CacaoPipelineCache> CreatePipelineCache(
            const std::vector<uint8_t>& initialData = {}) = 0;
        virtual Ref<CacaoGraphicsPipeline> CreateGraphicsPipeline(
            const GraphicsPipelineCreateInfo& info) = 0;
        virtual Ref<CacaoComputePipeline> CreateComputePipeline(
            const ComputePipelineCreateInfo& info) = 0;
        virtual Ref<CacaoSynchronization> CreateSynchronization(uint32_t maxFramesInFlight) = 0;
    };
}
#endif
