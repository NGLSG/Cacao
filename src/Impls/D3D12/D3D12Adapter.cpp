#include <utility>

#include "../../../include/Impls/D3D12/D3D12Adapter.h"

#include <iostream>
#include "Device.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Instance.h"
#ifdef WIN32
namespace Cacao
{
    std::string WideStringToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);
        return strTo;
    }

    AdapterProperties D3D12Adapter::GetProperties() const
    {
        return m_adapterProperties;
    }

    AdapterType D3D12Adapter::GetAdapterType() const
    {
        return m_adapterProperties.type;
    }

    bool D3D12Adapter::IsFeatureSupported(DeviceFeature feature) const
    {
        auto featureType = D3D12Converter::Convert(feature);

        switch (featureType)
        {
        // =====================================================================
        // OPTIONS
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                switch (feature)
                {
                // DX12 core shader stages
                case DeviceFeature::GeometryShader:
                case DeviceFeature::TessellationShader:
                    return true;

                // basic raster/draw guaranteed by DX12
                case DeviceFeature::MultiDrawIndirect:
                case DeviceFeature::FillModeNonSolid:
                case DeviceFeature::SamplerAnisotropy:
                case DeviceFeature::TextureCompressionBC:
                case DeviceFeature::ShaderFloat64:
                    return true;

                // not supported in D3D12
                case DeviceFeature::WideLines:
                case DeviceFeature::ConditionalRendering:
                    return false;

                // BC is core, ASTC not in this struct
                case DeviceFeature::BindlessDescriptors:
                    return data.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_2;

                case DeviceFeature::ShaderInt16:
                    return (data.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT) != 0;

                default:
                    return false;
                }
            }

        // =====================================================================
        // OPTIONS1 (Wave Ops = subgroup)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS1:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS1 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                switch (feature)
                {
                case DeviceFeature::SubgroupOperations:
                    return data.WaveOps;

                default:
                    return false;
                }
            }

        // =====================================================================
        // OPTIONS5 (Ray Tracing)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS5:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS5 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                bool rt = data.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;

                switch (feature)
                {
                case DeviceFeature::RayTracingPipeline:
                case DeviceFeature::RayTracingQuery:
                case DeviceFeature::AccelerationStructure:
                    return rt;

                default:
                    return false;
                }
            }

        // =====================================================================
        // OPTIONS6 (Variable Rate Shading)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS6:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS6 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                if (feature == DeviceFeature::VariableRateShading)
                    return data.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;

                return false;
            }

        // =====================================================================
        // OPTIONS7 (Mesh + Task)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS7:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS7 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                bool mesh = data.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;

                switch (feature)
                {
                case DeviceFeature::MeshShader:
                case DeviceFeature::TaskShader:
                    return mesh;

                default:
                    return false;
                }
            }

        // =====================================================================
        // OPTIONS8 (ASTC texture compression)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS8:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS8 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                if (feature == DeviceFeature::TextureCompressionASTC)
                    return false;

                return false;
            }

        // =====================================================================
        // OPTIONS12 (Buffer Device Address)
        // =====================================================================
        case D3D12_FEATURE_D3D12_OPTIONS12:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS12 data{};
                if (FAILED(m_tmpDevice->CheckFeatureSupport(featureType, &data, sizeof(data))))
                    return false;

                if (feature == DeviceFeature::BufferDeviceAddress)
                    return data.EnhancedBarriersSupported;

                return false;
            }

        default:
            return false;
        }
    }

    Ref<Device> D3D12Adapter::CreateDevice(const DeviceCreateInfo& info)
    {
        if (!m_dxgiAdapter)
        {
            throw std::runtime_error("D3D12Adapter::CreateDevice called on null adapter");
        }
        return D3D12Device::Create(shared_from_this(), info);
    }

    uint32_t D3D12Adapter::FindQueueFamilyIndex(QueueType type) const
    {
        //DX12无队列族的概念，直接返回0
        return 0;
    }

    const Ref<D3D12Instance>& D3D12Adapter::GetParentInstance()
    {
        return m_parentInstance;
    }

    D3D12Adapter::D3D12Adapter(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter)
    {
        if (!inst)
        {
            throw std::runtime_error("D3D12Adapter created with null instance");
        }
        m_dxgiAdapter = std::move(adapter);
        m_parentInstance = std::dynamic_pointer_cast<D3D12Instance>(inst);
        auto desc = DXGI_ADAPTER_DESC3{};
        m_dxgiAdapter->GetDesc3(&desc);
        m_adapterProperties.name = WideStringToUTF8(desc.Description);
        m_adapterProperties.vendorID = desc.VendorId;
        m_adapterProperties.deviceID = desc.DeviceId;
        m_adapterProperties.dedicatedVideoMemory = desc.DedicatedVideoMemory;
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
        {
            m_adapterProperties.type = AdapterType::Software;
        }
        else
        {
            if (desc.DedicatedVideoMemory > 0)
            {
                m_adapterProperties.type = AdapterType::Discrete;
            }
            else
            {
                m_adapterProperties.type = AdapterType::Integrated;
            }
        }
        if (FAILED(D3D12CreateDevice( m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_tmpDevice))))
        {
            std::cerr << "Failed to create temporary D3D12 device for adapter feature querying." << std::endl;
        }
    }

    Ref<D3D12Adapter> D3D12Adapter::Create(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter)
    {
        return CreateRef<D3D12Adapter>(inst, adapter);
    }
}
#endif
