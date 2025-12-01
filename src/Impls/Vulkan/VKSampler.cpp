#include "Impls/Vulkan/VKSampler.h"
#include "Impls/Vulkan/VKDevice.h"
namespace Cacao
{
    vk::Filter ConvertFilter(Filter filter)
    {
        switch (filter)
        {
        case Filter::Nearest:
            return vk::Filter::eNearest;
        case Filter::Linear:
            return vk::Filter::eLinear;
        default:
            return vk::Filter::eLinear;
        }
    }
    vk::SamplerAddressMode ConvertAddressMode(SamplerAddressMode addressMode)
    {
        switch (addressMode)
        {
        case SamplerAddressMode::Repeat:
            return vk::SamplerAddressMode::eRepeat;
        case SamplerAddressMode::MirroredRepeat:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case SamplerAddressMode::ClampToEdge:
            return vk::SamplerAddressMode::eClampToEdge;
        case SamplerAddressMode::ClampToBorder:
            return vk::SamplerAddressMode::eClampToBorder;
        case SamplerAddressMode::MirrorClampToEdge:
            return vk::SamplerAddressMode::eMirrorClampToEdge;
        default:
            return vk::SamplerAddressMode::eRepeat;
        }
    }
    vk::SamplerMipmapMode ConvertMipmapMode(SamplerMipmapMode mipmapMode)
    {
        switch (mipmapMode)
        {
        case SamplerMipmapMode::Nearest:
            return vk::SamplerMipmapMode::eNearest;
        case SamplerMipmapMode::Linear:
            return vk::SamplerMipmapMode::eLinear;
        default:
            return vk::SamplerMipmapMode::eLinear;
        }
    }
    vk::CompareOp ConvertCompareOp(CompareOp compareOp)
    {
        switch (compareOp)
        {
        case CompareOp::Never:
            return vk::CompareOp::eNever;
        case CompareOp::Less:
            return vk::CompareOp::eLess;
        case CompareOp::Equal:
            return vk::CompareOp::eEqual;
        case CompareOp::LessOrEqual:
            return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater:
            return vk::CompareOp::eGreater;
        case CompareOp::NotEqual:
            return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterOrEqual:
            return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::Always:
            return vk::CompareOp::eAlways;
        default:
            return vk::CompareOp::eAlways;
        }
    }
    vk::BorderColor ConvertBorderColor(BorderColor borderColor)
    {
        switch (borderColor)
        {
        case BorderColor::FloatTransparentBlack:
            return vk::BorderColor::eFloatTransparentBlack;
        case BorderColor::IntTransparentBlack:
            return vk::BorderColor::eIntTransparentBlack;
        case BorderColor::FloatOpaqueBlack:
            return vk::BorderColor::eFloatOpaqueBlack;
        case BorderColor::IntOpaqueBlack:
            return vk::BorderColor::eIntOpaqueBlack;
        case BorderColor::FloatOpaqueWhite:
            return vk::BorderColor::eFloatOpaqueWhite;
        case BorderColor::IntOpaqueWhite:
            return vk::BorderColor::eIntOpaqueWhite;
        default:
            return vk::BorderColor::eFloatOpaqueBlack;
        }
    }
    VKSampler::VKSampler(const Ref<CacaoDevice>& device, const SamplerCreateInfo& createInfo)
    {
        if (!device)
        {
            throw std::runtime_error("VKSampler created with null device");
        }
        m_device = std::static_pointer_cast<VKDevice>(device);
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.setAddressModeU(ConvertAddressMode(createInfo.AddressModeU))
                   .setAddressModeV(ConvertAddressMode(createInfo.AddressModeV))
                   .setAddressModeW(ConvertAddressMode(createInfo.AddressModeW))
                   .setMagFilter(ConvertFilter(createInfo.MagFilter))
                   .setMinFilter(ConvertFilter(createInfo.MinFilter))
                   .setMipmapMode(ConvertMipmapMode(createInfo.MipmapMode))
                   .setMipLodBias(createInfo.MipLodBias)
                   .setAnisotropyEnable(createInfo.AnisotropyEnable)
                   .setMaxAnisotropy(createInfo.MaxAnisotropy)
                   .setCompareEnable(createInfo.CompareEnable)
                   .setCompareOp(ConvertCompareOp(createInfo.CompareOp))
                   .setMinLod(createInfo.MinLod)
                   .setMaxLod(createInfo.MaxLod)
                   .setBorderColor(ConvertBorderColor(createInfo.BorderColor))
                   .setUnnormalizedCoordinates(VK_FALSE);
        m_sampler = m_device->GetHandle().createSampler(samplerInfo);
        if (!m_sampler)
        {
            throw std::runtime_error("Failed to create Vulkan sampler");
        }
    }
    Ref<VKSampler> VKSampler::Create(const Ref<CacaoDevice>& device, const SamplerCreateInfo& createInfo)
    {
        return CreateRef<VKSampler>(device, createInfo);
    }
    const SamplerCreateInfo& VKSampler::GetInfo() const
    {
        return m_createInfo;
    }
}
