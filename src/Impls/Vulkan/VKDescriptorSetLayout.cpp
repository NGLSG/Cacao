#include "Impls/Vulkan/VKDescriptorSetLayout.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKSampler.h"
namespace Cacao
{
    vk::DescriptorType ConvertDescriptorType(DescriptorType type)
    {
        switch (type)
        {
        case DescriptorType::Sampler:
            return vk::DescriptorType::eSampler;
        case DescriptorType::CombinedImageSampler:
            return vk::DescriptorType::eCombinedImageSampler;
        case DescriptorType::SampledImage:
            return vk::DescriptorType::eSampledImage;
        case DescriptorType::StorageImage:
            return vk::DescriptorType::eStorageImage;
        case DescriptorType::UniformBuffer:
            return vk::DescriptorType::eUniformBuffer;
        case DescriptorType::StorageBuffer:
            return vk::DescriptorType::eStorageBuffer;
        case DescriptorType::UniformBufferDynamic:
            return vk::DescriptorType::eUniformBufferDynamic;
        case DescriptorType::StorageBufferDynamic:
            return vk::DescriptorType::eStorageBufferDynamic;
        case DescriptorType::InputAttachment:
            return vk::DescriptorType::eInputAttachment;
        case DescriptorType::AccelerationStructure:
            return vk::DescriptorType::eAccelerationStructureKHR;
        default:
            return vk::DescriptorType::eSampler;
        }
    }
    inline vk::ShaderStageFlags ConvertShaderStage(ShaderStage stage)
    {
        vk::ShaderStageFlags shaderStage = vk::ShaderStageFlags();
        if (stage == ShaderStage::None)
            return shaderStage;
        auto stageBits = static_cast<uint32_t>(stage);
        if (stageBits & static_cast<uint32_t>(ShaderStage::Vertex))
            shaderStage |= vk::ShaderStageFlagBits::eVertex;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Fragment))
            shaderStage |= vk::ShaderStageFlagBits::eFragment;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Compute))
            shaderStage |= vk::ShaderStageFlagBits::eCompute;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Geometry))
            shaderStage |= vk::ShaderStageFlagBits::eGeometry;
        if (stageBits & static_cast<uint32_t>(ShaderStage::TessellationControl))
            shaderStage |= vk::ShaderStageFlagBits::eTessellationControl;
        if (stageBits & static_cast<uint32_t>(ShaderStage::TessellationEvaluation))
            shaderStage |= vk::ShaderStageFlagBits::eTessellationEvaluation;
#ifdef VK_ENABLE_BETA_EXTENSIONS
        if (stageBits & static_cast<uint32_t>(ShaderStage::Mesh))
            shaderStage |= vk::ShaderStageFlagBits::eMeshEXT;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Task))
            shaderStage |= vk::ShaderStageFlagBits::eTaskEXT;
#endif
#ifdef VK_KHR_ray_tracing_pipeline
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayGen))
            shaderStage |= vk::ShaderStageFlagBits::eRaygenKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayAnyHit))
            shaderStage |= vk::ShaderStageFlagBits::eAnyHitKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayClosestHit))
            shaderStage |= vk::ShaderStageFlagBits::eClosestHitKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayMiss))
            shaderStage |= vk::ShaderStageFlagBits::eMissKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::RayIntersection))
            shaderStage |= vk::ShaderStageFlagBits::eIntersectionKHR;
        if (stageBits & static_cast<uint32_t>(ShaderStage::Callable))
            shaderStage |= vk::ShaderStageFlagBits::eCallableKHR;
#endif
        return shaderStage;
    }
    std::vector<vk::Sampler> ConvertSamplers(const std::vector<Ref<CacaoSampler>>& samplers)
    {
        std::vector<vk::Sampler> vkSamplers;
        vkSamplers.reserve(samplers.size());
        for (const auto& sampler : samplers)
        {
            auto vkSampler = std::dynamic_pointer_cast<VKSampler>(sampler);
            vkSamplers.push_back(vkSampler->GetHandle());
        }
        return vkSamplers;
    }
    vk::DescriptorSetLayoutBinding ConvertBinding(const DescriptorSetLayoutBinding& binding)
    {
        vk::DescriptorSetLayoutBinding vkBinding{};
        vkBinding.binding = binding.Binding;
        vkBinding.descriptorType = ConvertDescriptorType(binding.Type);
        vkBinding.descriptorCount = binding.Count;
        vkBinding.stageFlags = ConvertShaderStage(binding.StageFlags);
        return vkBinding;
    }
    VKDescriptorSetLayout::VKDescriptorSetLayout(const Ref<CacaoDevice>& device,
                                                 const DescriptorSetLayoutCreateInfo& info)
    {
        if (!device)
        {
            throw std::runtime_error("VKDescriptorLayout created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
        std::vector<vk::DescriptorSetLayoutBinding> vkBindings(info.Bindings.size());
        m_samplerStorage.resize(info.Bindings.size());
        for (uint32_t i = 0; i < info.Bindings.size(); ++i)
        {
            vkBindings[i] = ConvertBinding(info.Bindings[i]);
            m_samplerStorage[i] = ConvertSamplers(info.Bindings[i].ImmutableSamplers);
            vkBindings[i].pImmutableSamplers = m_samplerStorage[i].data();
        }
        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        layoutCreateInfo.pBindings = vkBindings.data();
        m_descriptorSetLayout = m_device->GetHandle().createDescriptorSetLayout(layoutCreateInfo);
        if (!m_descriptorSetLayout)
        {
            throw std::runtime_error("Failed to create Vulkan Descriptor Set Layout");
        }
    }
    Ref<VKDescriptorSetLayout> VKDescriptorSetLayout::Create(const Ref<CacaoDevice>& device,
                                                             const DescriptorSetLayoutCreateInfo& info)
    {
        return CreateRef<VKDescriptorSetLayout>(device, info);
    }
}
