#include <Impls/Vulkan/VKAdapter.h>
#include "Instance.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKInstance.h"
namespace Cacao
{
    uint32_t VKAdapter::GetTotalGPUMemory() const
    {
        vk::PhysicalDeviceMemoryProperties memoryProperties = m_physicalDevice.getMemoryProperties();
        uint32_t totalMemory = 0;
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
        {
            if (memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
            {
                totalMemory += static_cast<uint32_t>(memoryProperties.memoryHeaps[i].size / (1024 * 1024));
            }
        }
        return totalMemory;
    }
    vk::PhysicalDevice VKAdapter::GetPhysicalDevice() const
    {
        return m_physicalDevice;
    }
    VKAdapter::VKAdapter(const Ref<Instance>& inst, const vk::PhysicalDevice& physicalDevice) : m_physicalDevice(
        physicalDevice)
    {
        if (!m_physicalDevice)
        {
            throw std::runtime_error("Invalid physical device provided to Adapter");
        }
        m_instance = std::dynamic_pointer_cast<VKInstance>(inst);
        vk::PhysicalDeviceProperties m_physicalDeviceProperties = m_physicalDevice.getProperties();
        m_properties.name = m_physicalDeviceProperties.deviceName.data();
        m_properties.vendorID = m_physicalDeviceProperties.vendorID;
        m_properties.deviceID = m_physicalDeviceProperties.deviceID;
        uint32_t totalMemoryMB = GetTotalGPUMemory();
        m_properties.dedicatedVideoMemory = static_cast<uint64_t>(totalMemoryMB) * 1024 * 1024;
        switch (m_physicalDeviceProperties.deviceType)
        {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            m_adapterType = AdapterType::Discrete;
            break;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            m_adapterType = AdapterType::Integrated;
            break;
        case vk::PhysicalDeviceType::eCpu:
            m_adapterType = AdapterType::Software;
            break;
        default:
            m_adapterType = AdapterType::Unknown;
        }
    }
    Ref<VKAdapter> VKAdapter::Create(const Ref<Instance>& inst, const vk::PhysicalDevice& physicalDevice)
    {
        return CreateRef<VKAdapter>(inst, physicalDevice);
    }
    AdapterProperties VKAdapter::GetProperties() const
    {
        return m_properties;
    }
    AdapterType VKAdapter::GetAdapterType() const
    {
        return m_adapterType;
    }
    bool VKAdapter::IsFeatureSupported(CacaoFeature feature) const
    {
        vk::PhysicalDeviceFeatures m_features = m_physicalDevice.getFeatures();
        switch (feature)
        {
        case CacaoFeature::RayTracing:
            {
                vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures{};
                vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&rayTracingFeatures);
                rayTracingFeatures.setPNext(&accelerationStructureFeatures);
                m_physicalDevice.getFeatures2(&features2);
                return rayTracingFeatures.rayTracingPipeline && accelerationStructureFeatures.accelerationStructure;
            }
        case CacaoFeature::MeshShader:
            {
                vk::PhysicalDeviceMeshShaderFeaturesNV meshShaderFeatures{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&meshShaderFeatures);
                m_physicalDevice.getFeatures2(&features2);
                return meshShaderFeatures.meshShader && meshShaderFeatures.taskShader;
            }
        case CacaoFeature::VariableRateShading:
            {
                vk::PhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatureKHR{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&fragmentShadingRateFeatureKHR);
                m_physicalDevice.getFeatures2(&features2);
                return fragmentShadingRateFeatureKHR.pipelineFragmentShadingRate;
            }
        case CacaoFeature::BindlessDescriptors:
            {
                vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&descriptorIndexingFeatures);
                m_physicalDevice.getFeatures2(&features2);
                return descriptorIndexingFeatures.descriptorBindingPartiallyBound &&
                    descriptorIndexingFeatures.runtimeDescriptorArray;
            }
        case CacaoFeature::BufferDeviceAddress:
            {
                vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&bufferDeviceAddressFeatures);
                m_physicalDevice.getFeatures2(&features2);
                return bufferDeviceAddressFeatures.bufferDeviceAddress;
            }
        case CacaoFeature::DrawIndirectCount:
            {
                return m_features.drawIndirectFirstInstance;
            }
        case CacaoFeature::ShaderFloat16:
            {
                vk::PhysicalDeviceShaderFloat16Int8Features float16Feature{};
                vk::PhysicalDeviceFeatures2 features2{};
                features2.setPNext(&float16Feature);
                m_physicalDevice.getFeatures2(&features2);
                return float16Feature.shaderFloat16;
            }
        case CacaoFeature::ShaderInt64:
            return m_features.shaderInt64;
        case CacaoFeature::GeometryShader:
            return m_features.geometryShader;
        case CacaoFeature::TessellationShader:
            return m_features.tessellationShader;
        case CacaoFeature::MultiViewport:
            return m_features.multiViewport;
        case CacaoFeature::IndependentBlending:
            return m_features.independentBlend;
        case CacaoFeature::PipelineStatistics:
            return m_features.pipelineStatisticsQuery;
        default:
            return false;
        }
    }
    std::shared_ptr<Device> VKAdapter::CreateDevice(const DeviceCreateInfo& info)
    {
        return VKDevice::Create(shared_from_this(), info);
    }
    uint32_t VKAdapter::FindQueueFamilyIndex(QueueType type) const
    {
        auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueFamilies.size(); i++)
        {
            const auto& queueFamily = queueFamilies[i];
            switch (type)
            {
            case QueueType::Graphics:
                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    return i;
                }
                break;
            case QueueType::Compute:
                if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
                {
                    return i;
                }
                break;
            case QueueType::Transfer:
                if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
                {
                    return i;
                }
                break;
            default:
                break;
            }
        }
        return UINT32_MAX;
    }
} 
