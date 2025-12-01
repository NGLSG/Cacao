#ifndef CACAO_VKDESCRIPTORPOOL_H
#define CACAO_VKDESCRIPTORPOOL_H
#include "CacaoDescriptorPool.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class CacaoDevice;
    class VKDevice;
    class CACAO_API VKDescriptorPool : public CacaoDescriptorPool
    {
        Ref<VKDevice> m_device;
        vk::DescriptorPool m_descriptorPool;
        DescriptorPoolCreateInfo m_createInfo;
    public:
        VKDescriptorPool(const Ref<CacaoDevice>& device, const DescriptorPoolCreateInfo& createInfo);
        static Ref<VKDescriptorPool> Create(const Ref<CacaoDevice>& device, const DescriptorPoolCreateInfo& createInfo);
        void Reset() override;
        Ref<CacaoDescriptorSet> AllocateDescriptorSet(const Ref<CacaoDescriptorSetLayout>& layout) override;
    };
}
#endif 
