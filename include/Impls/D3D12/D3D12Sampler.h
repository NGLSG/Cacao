#ifndef CACAO_D3D12SAMPLER_H
#define CACAO_D3D12SAMPLER_H
#ifdef WIN32
#include "Sampler.h"
#include "D3D12Common.h"
#include "Device.h"

namespace Cacao
{
    class D3D12Device;
    class D3D12DescriptorSet;
    class D3D12DescriptorSetLayout;

    class CACAO_API D3D12Sampler : public Sampler
    {
    private:
        Ref<D3D12Device> m_device;
        SamplerCreateInfo m_createInfo;
        D3D12_SAMPLER_DESC m_samplerDesc;

        friend class D3D12DescriptorSet;
        friend class D3D12DescriptorSetLayout;

    public:
        D3D12Sampler(const Ref<Device>& device, const SamplerCreateInfo& createInfo);
        static Ref<D3D12Sampler> Create(const Ref<Device>& device, const SamplerCreateInfo& createInfo);

        const SamplerCreateInfo& GetInfo() const override;

        const D3D12_SAMPLER_DESC& GetSamplerDesc() const
        {
            return m_samplerDesc;
        }


        ~D3D12Sampler() = default;
    };
}
#endif
#endif
