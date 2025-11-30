#include <Impls/Vulkan/VKDevice.h>
#include "CacaoBuffer.h"
#include "Impls/Vulkan/VKAdapter.h"
#include "Impls/Vulkan/VKBuffer.h"
#include "Impls/Vulkan/VKCommandBufferEncoder.h"
#include "Impls/Vulkan/VKInstance.h"
#include "Impls/Vulkan/VKQueue.h"
#include "Impls/Vulkan/VKSurface.h"
#include "Impls/Vulkan/VKSwapchain.h"
#include "Impls/Vulkan/VKTexture.h"
namespace Cacao
{
    Ref<VKDevice> VKDevice::Create(const Ref<CacaoAdapter>& adapter, const CacaoDeviceCreateInfo& createInfo)
    {
        auto device = CreateRef<VKDevice>(adapter, createInfo);
        return device;
    }
    VKDevice::VKDevice(const Ref<CacaoAdapter>& adapter, const CacaoDeviceCreateInfo& createInfo)
    {
        if (!adapter)
        {
            throw std::runtime_error("VKDevice created with null adapter");
        }
        m_parentAdapter = adapter;
        auto pyDevice = std::dynamic_pointer_cast<VKAdapter>(adapter)->GetPhysicalDevice();
        int presentQueueFamily = -1;
        if (createInfo.CompatibleSurface)
            presentQueueFamily = std::dynamic_pointer_cast<VKSurface>(createInfo.CompatibleSurface)->
                GetPresentQueueFamilyIndex(adapter);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (const auto& queueRequest : createInfo.QueueRequests)
        {
            uint32_t queueFamilyIndex = std::dynamic_pointer_cast<VKAdapter>(adapter)->FindQueueFamilyIndex(
                queueRequest.Type);
            vk::DeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = queueRequest.Count;
            std::vector<float> priorities(queueRequest.Count, queueRequest.Priority);
            queueCreateInfo.pQueuePriorities = priorities.data();
            queueCreateInfos.push_back(queueCreateInfo);
            m_queueFamilyIndices.push_back(queueFamilyIndex);
        }
        vk::PhysicalDeviceFeatures features10{};
        vk::PhysicalDeviceVulkan12Features features12{};
        vk::PhysicalDeviceVulkan13Features features13{};
        features13.dynamicRendering = VK_TRUE;
        vk::PhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
        vk::PhysicalDeviceFragmentShadingRateFeaturesKHR vrsFeatures{};
        std::vector<const char*> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_MAINTENANCE2_EXTENSION_NAME,
            VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
        };
        for (const auto& feature : createInfo.EnabledFeatures)
        {
            switch (feature)
            {
            case CacaoDeviceFeature::GeometryShader:
                features10.geometryShader = VK_TRUE;
                break;
            case CacaoDeviceFeature::TessellationShader:
                features10.tessellationShader = VK_TRUE;
                break;
            case CacaoDeviceFeature::MultiDrawIndirect:
                features10.multiDrawIndirect = VK_TRUE;
                break;
            case CacaoDeviceFeature::FillModeNonSolid:
                features10.fillModeNonSolid = VK_TRUE;
                break;
            case CacaoDeviceFeature::WideLines:
                features10.wideLines = VK_TRUE;
                break;
            case CacaoDeviceFeature::SamplerAnisotropy:
                features10.samplerAnisotropy = VK_TRUE;
                break;
            case CacaoDeviceFeature::BindlessDescriptors:
                features12.descriptorIndexing = VK_TRUE;
                features12.runtimeDescriptorArray = VK_TRUE;
                features12.descriptorBindingPartiallyBound = VK_TRUE;
                features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
                features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                break;
            case CacaoDeviceFeature::BufferDeviceAddress:
                features12.bufferDeviceAddress = VK_TRUE;
                extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
                break;
            case CacaoDeviceFeature::TextureCompressionBC:
                features10.textureCompressionBC = VK_TRUE;
                break;
            case CacaoDeviceFeature::TextureCompressionASTC:
                features10.textureCompressionASTC_LDR = VK_TRUE;
                break;
            case CacaoDeviceFeature::MeshShader:
                meshFeatures.meshShader = VK_TRUE;
                meshFeatures.taskShader = VK_TRUE;
                extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
                break;
            case CacaoDeviceFeature::RayTracingPipeline:
                rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
                extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                break;
            case CacaoDeviceFeature::AccelerationStructure:
                asFeatures.accelerationStructure = VK_TRUE;
                extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                break;
            case CacaoDeviceFeature::VariableRateShading:
                vrsFeatures.pipelineFragmentShadingRate = VK_TRUE;
                extensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
                break;
            case CacaoDeviceFeature::ShaderFloat64:
                features10.shaderFloat64 = VK_TRUE;
                break;
            case CacaoDeviceFeature::ShaderInt16:
                features10.shaderInt16 = VK_TRUE;
                break;
            default:
                break;
            }
        }
        void* pNextChain = nullptr;
        auto ChainStruct = [&](auto& structure)
        {
            structure.pNext = pNextChain;
            pNextChain = &structure;
        };
        ChainStruct(features12);
        ChainStruct(features13);
        if (std::find(extensions.begin(), extensions.end(), VK_EXT_MESH_SHADER_EXTENSION_NAME) != extensions.end())
        {
            ChainStruct(meshFeatures);
        }
        if (std::find(extensions.begin(), extensions.end(), VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) != extensions.
            end())
        {
            ChainStruct(rtPipelineFeatures);
        }
        if (std::find(extensions.begin(), extensions.end(), VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) != extensions.
            end())
        {
            ChainStruct(asFeatures);
        }
        if (std::find(extensions.begin(), extensions.end(), VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME) != extensions.
            end())
        {
            ChainStruct(vrsFeatures);
        }
        vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo().setEnabledExtensionCount(
                                                                           static_cast<uint32_t>(extensions.size())).
                                                                       setPpEnabledExtensionNames(extensions.data()).
                                                                       setQueueCreateInfoCount(
                                                                           static_cast<uint32_t>(queueCreateInfos.
                                                                               size())).
                                                                       setPQueueCreateInfos(queueCreateInfos.data()).
                                                                       setPEnabledFeatures(&features10).
                                                                       setPNext(pNextChain);
        m_Device = pyDevice.createDevice(deviceCreateInfo);
        if (!m_Device)
        {
            throw std::runtime_error("Failed to create Vulkan logical device");
        }
        vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo().setFlags(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer).setQueueFamilyIndex(
            std::dynamic_pointer_cast<VKAdapter>(adapter)->FindQueueFamilyIndex(QueueType::Graphics));
        m_graphicsCommandPool = m_Device.createCommandPool(commandPoolCreateInfo);
        if (!m_graphicsCommandPool)
        {
            throw std::runtime_error("Failed to create graphics command pool");
        }
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = std::dynamic_pointer_cast<VKAdapter>(m_parentAdapter)->GetPhysicalDevice();
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = std::dynamic_pointer_cast<VKAdapter>(m_parentAdapter)->GetInstance()->
            GetVulkanInstance();
        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }
    void VKDevice::WaitIdle() const
    {
        m_Device.waitIdle();
    }
    Ref<CacaoQueue> VKDevice::GetQueue(QueueType type, uint32_t index)
    {
        uint32_t familyIndex = m_parentAdapter->FindQueueFamilyIndex(type);
        vk::Queue vkQueue = m_Device.getQueue(familyIndex, index);
        return VKQueue::Create(shared_from_this(), vkQueue, type, index);
    }
    Ref<CacaoSwapchain> VKDevice::CreateSwapchain(const SwapchainCreateInfo& createInfo)
    {
        return VKSwapchain::Create(shared_from_this(), createInfo);
    }
    std::vector<uint32_t> VKDevice::GetAllQueueFamilyIndices() const
    {
        return m_queueFamilyIndices;
    }
    Ref<CacaoAdapter> VKDevice::GetParentAdapter() const
    {
        return m_parentAdapter;
    }
    Ref<CacaoCommandBufferEncoder> VKDevice::CreateCommandBufferEncoder(
        CommandBufferType type)
    {
        switch (type)
        {
        case CommandBufferType::Primary:
            {
                if (m_primCommandBuffers.empty())
                {
                    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                                 .setCommandPool(m_graphicsCommandPool)
                                                                 .setLevel(vk::CommandBufferLevel::ePrimary)
                                                                 .setCommandBufferCount(1);
                    vk::CommandBuffer commandBuffer = m_Device.allocateCommandBuffers(allocateInfo).front();
                    m_primCommandBuffers.push(
                        VKCommandBufferEncoder::Create(shared_from_this(), commandBuffer, CommandBufferType::Primary));
                }
                auto commandBuffer = m_primCommandBuffers.front();
                m_primCommandBuffers.pop();
                return commandBuffer;
            }
        case CommandBufferType::Secondary:
            {
                if (m_secCommandBuffers.empty())
                {
                    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                                 .setCommandPool(m_graphicsCommandPool)
                                                                 .setLevel(vk::CommandBufferLevel::eSecondary)
                                                                 .setCommandBufferCount(1);
                    vk::CommandBuffer commandBuffer = m_Device.allocateCommandBuffers(allocateInfo).front();
                    m_secCommandBuffers.push(
                        VKCommandBufferEncoder::Create(shared_from_this(), commandBuffer,
                                                       CommandBufferType::Secondary));
                }
                auto commandBuffer = m_secCommandBuffers.front();
                m_secCommandBuffers.pop();
                return commandBuffer;
            }
        default:
            throw std::runtime_error("Unsupported CommandBufferType in CreateCommandBufferEncoder");
        }
    }
    void VKDevice::ResetCommandPool()
    {
        m_Device.resetCommandPool(m_graphicsCommandPool);
    }
    void VKDevice::ReturnCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder)
    {
        if (encoder->GetCommandBufferType() == CommandBufferType::Primary)
        {
            m_primCommandBuffers.push(std::dynamic_pointer_cast<VKCommandBufferEncoder>(encoder));
        }
        else
        {
            m_secCommandBuffers.push(std::dynamic_pointer_cast<VKCommandBufferEncoder>(encoder));
        }
    }
    void VKDevice::FreeCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder)
    {
        auto cmdBuffer = std::dynamic_pointer_cast<VKCommandBufferEncoder>(encoder);
        m_Device.freeCommandBuffers(m_graphicsCommandPool, cmdBuffer->GetVulkanCommandBuffer());
    }
    void VKDevice::ResetCommandBuffer(const Ref<CacaoCommandBufferEncoder>& encoder)
    {
        auto cmdBuffer = std::dynamic_pointer_cast<VKCommandBufferEncoder>(encoder);
        cmdBuffer->GetVulkanCommandBuffer().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    }
    Ref<CacaoTexture> VKDevice::CreateTexture(const TextureCreateInfo& createInfo)
    {
        return VKTexture::Create(shared_from_this(), m_allocator, createInfo);
    }
    Ref<CacaoBuffer> VKDevice::CreateBuffer(const BufferCreateInfo& createInfo)
    {
        return VKBuffer::Create(shared_from_this(), m_allocator, createInfo);
    }
} 
