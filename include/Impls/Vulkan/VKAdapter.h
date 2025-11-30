#ifndef CACAO_VKADAPTER_H
#define CACAO_VKADAPTER_H
#include "../../CacaoAdapter.h"
#include  <vulkan/vulkan.hpp>
namespace Cacao
{
    class CacaoInstance;
}
namespace Cacao
{
    class CACAO_API VKAdapter : public CacaoAdapter
    {
    private:
        friend class VKSurface;
        friend class VKInstance;
        friend class VKDevice;
        vk::PhysicalDevice m_physicalDevice;
        AdapterProperties m_properties;
        AdapterType m_adapterType;
        uint32_t GetTotalGPUMemory() const;
        vk::PhysicalDevice GetPhysicalDevice() const;
        Ref<VKInstance> m_instance;
        Ref<VKInstance>& GetInstance() { return m_instance; }
    public:
        VKAdapter(const Ref<CacaoInstance>& inst, const vk::PhysicalDevice& physicalDevice);
        static Ref<VKAdapter> Create(const Ref<CacaoInstance>& inst,const vk::PhysicalDevice& physicalDevice);
        AdapterProperties GetProperties() const override;
        AdapterType GetAdapterType() const override;
        bool IsFeatureSupported(CacaoFeature feature) const override;
        std::shared_ptr<CacaoDevice> CreateDevice(const CacaoDeviceCreateInfo& info) override;
        uint32_t FindQueueFamilyIndex(QueueType type) const override;
    };
} 
#endif 
