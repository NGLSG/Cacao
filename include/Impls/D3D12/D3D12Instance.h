#ifndef CACAO_D3D12INSTANCE_H
#define CACAO_D3D12INSTANCE_H
#ifdef WIN32
#include "Instance.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Instance : public Instance
    {
    private:
        ComPtr<IDXGIFactory7> m_dxgiFactory;
        InstanceCreateInfo m_createInfo;
    public:
        [[nodiscard]] BackendType GetType() const override;
        bool Initialize(const InstanceCreateInfo& createInfo) override;
        std::vector<Ref<Adapter>> EnumerateAdapters() override;
        bool IsFeatureEnabled(InstanceFeature feature) const override;
        Ref<Surface> CreateSurface(const NativeWindowHandle& windowHandle) override;
        Ref<ShaderCompiler> CreateShaderCompiler() override;
        ComPtr<IDXGIFactory7>& GetDXGIFactory();
    };
}
#endif
#endif 
