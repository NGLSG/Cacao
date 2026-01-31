#ifdef WIN32
#include "Impls/D3D12/D3D12DescriptorSetLayout.h"
#include "Impls/D3D12/D3D12Device.h"
#include <stdexcept>

namespace Cacao
{
    D3D12DescriptorSetLayout::D3D12DescriptorSetLayout(const Ref<Device>& device, 
                                                        const DescriptorSetLayoutCreateInfo& info)
        : m_createInfo(info)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12DescriptorSetLayout created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12DescriptorSetLayout requires a D3D12Device");
        }

        // 计算每种类型的描述符数量
        // D3D12 需要将 CBV/SRV/UAV 和 Sampler 分开管理
        for (const auto& binding : info.Bindings)
        {
            D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12Converter::Convert(binding.Type);
            
            if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
            {
                m_samplerCount += binding.Count;
            }
            else
            {
                m_cbvSrvUavCount += binding.Count;
            }
        }
    }

    Ref<DescriptorSetLayout> D3D12DescriptorSetLayout::Create(const Ref<Device>& device, 
                                                               const DescriptorSetLayoutCreateInfo& info)
    {
        return CreateRef<D3D12DescriptorSetLayout>(device, info);
    }
}
#endif
