#ifndef CACAO_D3D12ADAPTER_H
#define CACAO_D3D12ADAPTER_H
#ifdef WIN32
#include "D3D12Common.h"
#include "Adapter.h"

namespace Cacao
{
    class Instance;
}

namespace Cacao
{
    class CACAO_API D3D12Adapter : public Adapter
    {
        ComPtr<IDXGIAdapter4> m_dxgiAdapter;

    public:
        AdapterProperties GetProperties() const override;
        AdapterType GetAdapterType() const override;
        bool IsFeatureSupported(CacaoFeature feature) const override;
        std::shared_ptr<Device> CreateDevice(const DeviceCreateInfo& info) override;
        uint32_t FindQueueFamilyIndex(QueueType type) const override;
        D3D12Adapter(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter);
        static Ref<D3D12Adapter> Create(const Ref<Instance>& inst, ComPtr<IDXGIAdapter4> adapter);
    };
}
#endif
#endif
