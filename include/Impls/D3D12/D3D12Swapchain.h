#ifndef CACAO_D3D12SWAPCHAIN_H
#define CACAO_D3D12SWAPCHAIN_H
#ifdef WIN32
#include "Swapchain.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Swapchain: public Swapchain
    {
    public:
        Result Present(const Ref<Queue>& queue, const Ref<Synchronization>& sync, uint32_t frameIndex) override;
        uint32_t GetImageCount() const override;
        Ref<Texture> GetBackBuffer(uint32_t index) const override;
        Extent2D GetExtent() const override;
        Format GetFormat() const override;
        PresentMode GetPresentMode() const override;
        Result AcquireNextImage(const Ref<Synchronization>& sync, int idx, int& out) override;
    private:
        ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
    public:
    };
} 
#endif
#endif 
