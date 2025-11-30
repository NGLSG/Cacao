#ifndef CACAO_CACAODEVICE_H
#define CACAO_CACAODEVICE_H
namespace Cacao
{
    struct BufferCreateInfo;
    class CacaoBuffer;
    struct TextureCreateInfo;
    class CacaoTexture;
}
namespace Cacao
{
    class CacaoCommandBufferEncoder;
}
namespace Cacao
{
    class CacaoAdapter;
}
namespace Cacao
{
    struct SwapchainCreateInfo;
    class CacaoSwapchain;
}
namespace Cacao
{
    class CacaoQueue;
}
namespace Cacao
{
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
        const Ref<CacaoSurface> CompatibleSurface = nullptr;
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
    };
} 
#endif 
