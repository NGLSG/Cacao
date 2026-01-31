#ifndef CACAO_D3D12SURFACE_H
#define CACAO_D3D12SURFACE_H
#ifdef WIN32
#include "Instance.h"
#include "Surface.h"
#include "D3D12Common.h"

namespace Cacao
{
    class CACAO_API D3D12Surface : public Surface
    {
        HWND m_hwnd = nullptr;
        
    public:
        static Ref<D3D12Surface> Create(const Ref<Instance>& inst, const NativeWindowHandle& windowHandle);
        D3D12Surface(const Ref<Instance>& inst, const NativeWindowHandle& windowHandle);

        SurfaceCapabilities GetCapabilities(const Ref<Adapter>& adapter) override;
        std::vector<SurfaceFormat> GetSupportedFormats(const Ref<Adapter>& adapter) override;
        Ref<Queue> GetPresentQueue(const Ref<Device>& device) override;
        std::vector<PresentMode> GetSupportedPresentModes(const Ref<Adapter>& adapter) override;
        
        HWND GetHWND() const { return m_hwnd; }
    };
}
#endif
#endif
