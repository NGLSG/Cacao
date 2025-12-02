#include "Impls/Vulkan/VKSampler.h"
#include "Impls/Vulkan/VKConvert.h"
#include "Impls/Vulkan/VKDevice.h"
namespace Cacao
{
    VKSampler::VKSampler(const Ref<Device>& device, const SamplerCreateInfo& createInfo)
    {
        if (!device)
        {
            throw std::runtime_error("VKSampler created with null device");
        }
        m_device = std::static_pointer_cast<VKDevice>(device);
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.setAddressModeU(Converter::Convert(createInfo.AddressModeU))
                   .setAddressModeV(Converter::Convert(createInfo.AddressModeV))
                   .setAddressModeW(Converter::Convert(createInfo.AddressModeW))
                   .setMagFilter(Converter::Convert(createInfo.MagFilter))
                   .setMinFilter(Converter::Convert(createInfo.MinFilter))
                   .setMipmapMode(Converter::Convert(createInfo.MipmapMode))
                   .setMipLodBias(createInfo.MipLodBias)
                   .setAnisotropyEnable(createInfo.AnisotropyEnable)
                   .setMaxAnisotropy(createInfo.MaxAnisotropy)
                   .setCompareEnable(createInfo.CompareEnable)
                   .setCompareOp(Converter::Convert(createInfo.CompareOp))
                   .setMinLod(createInfo.MinLod)
                   .setMaxLod(createInfo.MaxLod)
                   .setBorderColor(Converter::Convert(createInfo.BorderColor))
                   .setUnnormalizedCoordinates(VK_FALSE);
        m_sampler = m_device->GetHandle().createSampler(samplerInfo);
        if (!m_sampler)
        {
            throw std::runtime_error("Failed to create Vulkan sampler");
        }
    }
    Ref<VKSampler> VKSampler::Create(const Ref<Device>& device, const SamplerCreateInfo& createInfo)
    {
        return CreateRef<VKSampler>(device, createInfo);
    }
    const SamplerCreateInfo& VKSampler::GetInfo() const
    {
        return m_createInfo;
    }
}
