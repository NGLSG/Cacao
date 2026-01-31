#ifndef CACAO_D3D12ADAPTER_H
#define CACAO_D3D12ADAPTER_H
#ifdef WIN32
#include "D3D12Common.h"
#include "Adapter.h"

namespace Cacao
{
    class D3D12Instance;
    class Instance;

    class CACAO_API D3D12Adapter : public Adapter
    {
        ComPtr<IDXGIAdapter4> m_dxgiAdapter;
        AdapterProperties m_adapterProperties;
        mutable ComPtr<ID3D12Device14> m_tmpDevice;
        Ref<D3D12Instance> m_parentInstance;

    public:
        AdapterProperties GetProperties() const override;
        AdapterType GetAdapterType() const override;
        bool IsFeatureSupported(DeviceFeature feature) const override;
        Ref<Device> CreateDevice(const DeviceCreateInfo& info) override;
        uint32_t FindQueueFamilyIndex(QueueType type) const override;
        const Ref<D3D12Instance>& GetParentInstance();
        D3D12Adapter(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter);
        static Ref<D3D12Adapter> Create(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter);
        const ComPtr<IDXGIAdapter4>& GetDXGIAdapter() { return m_dxgiAdapter; }
    };
}
#endif
#endif
