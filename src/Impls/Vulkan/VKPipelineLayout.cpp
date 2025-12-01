#include "Impls/Vulkan/VKPipelineLayout.h"
#include "Impls/Vulkan/VKDescriptorSetLayout.h"
#include "Impls/Vulkan/VKDevice.h"
namespace Cacao
{
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
    VKPipelineLayout::VKPipelineLayout(const Ref<CacaoDevice>& device, const PipelineLayoutCreateInfo& info)
    {
        if (!device)
        {
            throw std::runtime_error("VKPipelineLayout created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
        m_descriptorSetLayouts.resize(info.SetLayouts.size());
        for (size_t i = 0; i < info.SetLayouts.size(); i++)
        {
            if (!info.SetLayouts[i])
            {
                throw std::runtime_error("VKPipelineLayout created with null descriptor set layout");
            }
            m_descriptorSetLayouts[i] = std::dynamic_pointer_cast<VKDescriptorSetLayout>(info.SetLayouts[i])->
                GetHandle();
        }
        m_pushConstantRanges.resize(info.PushConstantRanges.size());
        for (size_t i = 0; i < info.PushConstantRanges.size(); i++)
        {
            const auto& range = info.PushConstantRanges[i];
            vk::PushConstantRange vkRange{};
            vkRange.offset = range.Offset;
            vkRange.size = range.Size;
            vkRange.stageFlags = ConvertShaderStage(range.StageFlags);
            m_pushConstantRanges[i] = vkRange;
        }
        vk::PipelineLayoutCreateInfo createInfo{};
        createInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
        createInfo.pSetLayouts = m_descriptorSetLayouts.data();
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
        createInfo.pPushConstantRanges = m_pushConstantRanges.data();
        m_pipelineLayout = m_device->GetHandle().createPipelineLayout(createInfo);
        if (!m_pipelineLayout)
        {
            throw std::runtime_error("Failed to create Vulkan Pipeline Layout");
        }
    }
    Ref<VKPipelineLayout> VKPipelineLayout::Create(const Ref<CacaoDevice>& device, const PipelineLayoutCreateInfo& info)
    {
        return CreateRef<VKPipelineLayout>(device, info);
    }
}
