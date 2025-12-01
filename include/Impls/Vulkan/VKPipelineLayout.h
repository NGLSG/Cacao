#ifndef CACAO_VKPIPELINELAYOUT_H
#define CACAO_VKPIPELINELAYOUT_H
#include "CacaoPipelineLayout.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class CacaoDevice;
}
namespace Cacao
{
    class VKDevice;
    class CACAO_API VKPipelineLayout : public CacaoPipelineLayout
    {
        vk::PipelineLayout m_pipelineLayout;
        Ref<VKDevice> m_device;
        std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
        std::vector<vk::PushConstantRange> m_pushConstantRanges;
    public:
        VKPipelineLayout(const Ref<CacaoDevice>& device, const PipelineLayoutCreateInfo& info);
        static Ref<VKPipelineLayout> Create(const Ref<CacaoDevice>& device, const PipelineLayoutCreateInfo& info);
        vk::PipelineLayout& GetHandle() { return m_pipelineLayout; }
    };
}
#endif 
