#include "../../../include/Impls/D3D12/D3D12Instance.h"

#include "ShaderCompiler.h"
#include "Impls/D3D12/D3D12Adapter.h"
#include "Impls/D3D12/D3D12Surface.h"
#ifdef WIN32
namespace Cacao
{
    BackendType D3D12Instance::GetType() const
    {
        return BackendType::DirectX12;
    }

    bool D3D12Instance::Initialize(const InstanceCreateInfo& createInfo)
    {
        m_createInfo = createInfo;
        UINT flags = 0;
        for (const auto& feature : m_createInfo.enabledFeatures)
        {
            if (feature == InstanceFeature::ValidationLayer)
            {
                flags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
        CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_dxgiFactory));
        if (!m_dxgiFactory)
        {
            return false;
        }
        return true;
    }

    std::vector<Ref<Adapter>> D3D12Instance::EnumerateAdapters()
    {
        ComPtr<IDXGIAdapter1> adapter;
        std::vector<Ref<Adapter>> adapters;
        for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            ComPtr<IDXGIAdapter4> adapter4;
            adapter.As<IDXGIAdapter4>(&adapter4);
            if ( !adapter4 )
            {
                continue;
            }
            adapters.push_back(D3D12Adapter::Create(shared_from_this(), adapter4));
        }
        return adapters;
    }

    bool D3D12Instance::IsFeatureEnabled(InstanceFeature feature) const
    {
        for (const auto& enabledFeature : m_createInfo.enabledFeatures)
        {
            if (enabledFeature == feature)
            {
                return true;
            }
        }
        return false;
    }

    Ref<Surface> D3D12Instance::CreateSurface(const NativeWindowHandle& windowHandle)
    {
        return D3D12Surface::Create(shared_from_this(), windowHandle);
    }

    Ref<ShaderCompiler> D3D12Instance::CreateShaderCompiler()
    {
        return ShaderCompiler::Create(GetType());
    }

    ComPtr<IDXGIFactory7>& D3D12Instance::GetDXGIFactory()
    {
        return m_dxgiFactory;
    }
}
#endif
