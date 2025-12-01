#ifndef CACAO_VKPIPELINE_H
#define CACAO_VKPIPELINE_H
#include "CacaoPipeline.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class VKDevice;
}
namespace Cacao
{
    class CacaoDevice;
    class CACAO_API VKPipelineCache : public CacaoPipelineCache
    {
    public:
        VKPipelineCache(const Ref<CacaoDevice>& device,
                        const std::vector<uint8_t>& initialData = {});
        ~VKPipelineCache() override;
        static Ref<VKPipelineCache> Create(const Ref<CacaoDevice>& device,
                                           const std::vector<uint8_t>& initialData = {});
        std::vector<uint8_t> GetData() const override;
        void Merge(const std::vector<Ref<CacaoPipelineCache>>& srcCaches) override;
        vk::PipelineCache& GetHandle() { return m_pipelineCache; }
    private:
        vk::PipelineCache m_pipelineCache;
        Ref<VKDevice> m_device;
    };
    class CACAO_API VKGraphicsPipeline final : public CacaoGraphicsPipeline
    {
        vk::Pipeline m_pipeline;
        GraphicsPipelineCreateInfo m_pipelineInfo;
        Ref<VKDevice> m_device;
        std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
        std::vector<vk::VertexInputBindingDescription> m_vertexBindings;
        std::vector<vk::VertexInputAttributeDescription> m_vertexAttributes;
        std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
        std::vector<vk::SampleMask> m_sampleMask;
        std::vector<vk::Format> m_colorAttachmentFormats;
        std::vector<vk::DynamicState> m_dynamicStates;
    public:
        VKGraphicsPipeline(const Ref<CacaoDevice>& device,
                           const GraphicsPipelineCreateInfo& createInfo);
        ~VKGraphicsPipeline() override;
        static Ref<VKGraphicsPipeline> Create(const Ref<CacaoDevice>& device,
                                              const GraphicsPipelineCreateInfo& createInfo);
        Ref<CacaoPipelineLayout> GetLayout() const override;
        vk::Pipeline& GetHandle() { return m_pipeline; }
    };
    class CACAO_API VKComputePipeline : public CacaoComputePipeline
    {
    public:
        VKComputePipeline(const Ref<CacaoDevice>& device,
                          const ComputePipelineCreateInfo& createInfo);
        ~VKComputePipeline() override;
        static Ref<VKComputePipeline> Create(const Ref<CacaoDevice>& device,
                                             const ComputePipelineCreateInfo& createInfo);
        Ref<CacaoPipelineLayout> GetLayout() const override;
        vk::Pipeline GetHandle() const { return m_pipeline; }
    private:
        vk::Pipeline m_pipeline;
        ComputePipelineCreateInfo m_pipelineInfo;
        Ref<VKDevice> m_device;
    };
}
#endif 
