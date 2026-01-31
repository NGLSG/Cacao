#ifndef CACAO_D3D12DESCRIPTORLAYOUT_H
#define CACAO_D3D12DESCRIPTORLAYOUT_H
#ifdef WIN32
#include "DescriptorSetLayout.h"
#include "D3D12Common.h"

namespace Cacao
{
    class D3D12Device;

    class CACAO_API D3D12DescriptorSetLayout : public DescriptorSetLayout
    {
        Ref<D3D12Device> m_device;
        DescriptorSetLayoutCreateInfo m_createInfo;
        
        // 存储描述符堆中需要的描述符数量
        uint32_t m_cbvSrvUavCount = 0;
        uint32_t m_samplerCount = 0;

    public:
        const std::vector<DescriptorSetLayoutBinding>& GetBindings() const { return m_createInfo.Bindings; }
        const DescriptorSetLayoutCreateInfo& GetCreateInfo() const { return m_createInfo; }
        uint32_t GetCbvSrvUavCount() const { return m_cbvSrvUavCount; }
        uint32_t GetSamplerCount() const { return m_samplerCount; }

        static Ref<DescriptorSetLayout> Create(const Ref<Device>& device, const DescriptorSetLayoutCreateInfo& info);
        D3D12DescriptorSetLayout(const Ref<Device>& device, const DescriptorSetLayoutCreateInfo& info);
        ~D3D12DescriptorSetLayout() override = default;
    };
}
#endif
#endif
