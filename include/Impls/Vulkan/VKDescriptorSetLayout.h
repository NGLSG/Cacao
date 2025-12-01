#ifndef CACAO_VKDESCRIPTOR_H
#define CACAO_VKDESCRIPTOR_H
#include "CacaoDescriptorSetLayout.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class VKDevice;
    class CacaoDevice;
    class CACAO_API VKDescriptorSetLayout : public CacaoDescriptorSetLayout
    {
        vk::DescriptorSetLayout m_descriptorSetLayout;
        Ref<VKDevice> m_device;
        std::vector<std::vector<vk::Sampler>> m_samplerStorage;
    public:
        VKDescriptorSetLayout(const Ref<CacaoDevice>& device, const DescriptorSetLayoutCreateInfo& info);
        static Ref<VKDescriptorSetLayout> Create(const Ref<CacaoDevice>& device,
                                                 const DescriptorSetLayoutCreateInfo& info);
        vk::DescriptorSetLayout& GetHandle()
        {
            return m_descriptorSetLayout;
        }
    };
}
#endif 
