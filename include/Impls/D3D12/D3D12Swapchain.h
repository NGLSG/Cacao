#ifndef CACAO_D3D12SWAPCHAIN_H
#define CACAO_D3D12SWAPCHAIN_H
#ifdef WIN32
#include "Swapchain.h"
#include "D3D12Common.h"

namespace Cacao
{
    class D3D12Device;
    class D3D12Texture;

    class CACAO_API D3D12Swapchain : public Swapchain
    {
        ComPtr<IDXGISwapChain4> m_swapchain;
        Ref<D3D12Device> m_device;
        SwapchainCreateInfo m_createInfo;
        
        std::vector<Ref<D3D12Texture>> m_backBuffers;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        
        uint32_t m_currentBackBufferIndex = 0;
        bool m_tearingSupported = false;

    public:
        D3D12Swapchain(const Ref<Device>& device, const SwapchainCreateInfo& createInfo);
        ~D3D12Swapchain() override;
        
        Result Present(const Ref<Queue>& queue, const Ref<Synchronization>& sync, uint32_t frameIndex) override;
        uint32_t GetImageCount() const override;
        Ref<Texture> GetBackBuffer(uint32_t index) const override;
        Extent2D GetExtent() const override;
        Format GetFormat() const override;
        PresentMode GetPresentMode() const override;
        Result AcquireNextImage(const Ref<Synchronization>& sync, int idx, int& out) override;
        
        IDXGISwapChain4* GetHandle() const { return m_swapchain.Get(); }
        uint32_t GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const { return m_rtvHandles[m_currentBackBufferIndex]; }
        
        static Ref<Swapchain> Create(const Ref<Device>& device, const SwapchainCreateInfo& createInfo);
    };
}
#endif
#endif
