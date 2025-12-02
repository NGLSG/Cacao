#ifndef CACAO_D3D12SURFACE_H
#define CACAO_D3D12SURFACE_H
#include <complex.h>

#include "Instance.h"
#ifdef WIN32
#include "Surface.h"
#include "D3D12Common.h"

namespace Cacao
{
    class CACAO_API D3D12Surface : public Surface
    {
        ComPtr<IDXGISurface2> m_dxgiSurface;

    public:
        SurfaceCapabilities GetCapabilities(const Ref<Adapter>& adapter) override;
        std::vector<SurfaceFormat> GetSupportedFormats(const Ref<Adapter>& adapter) override;
        Ref<Queue> GetPresentQueue(const Ref<Device>& device) override;
        std::vector<PresentMode> GetSupportedPresentModes(const Ref<Adapter>& adapter) override;
        static Ref<Surface> Create(const Ref<Instance>& inst, NativeWindowHandle window_handle);
    };
}
#endif
#endif
