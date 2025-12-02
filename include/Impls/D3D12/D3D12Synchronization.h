#ifndef CACAO_D3D12SYNCHRONIZATION_H
#define CACAO_D3D12SYNCHRONIZATION_H
#ifdef WIN32
#include "Synchronization.h"
#include  "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Synchronization : public Synchronization
    {
    public:
        void WaitForFrame(uint32_t frameIndex) const override;
        void ResetFrameFence(uint32_t frameIndex) const override;
        uint32_t AcquireNextImageIndex(const Ref<Swapchain>& swapchain, uint32_t frameIndex) const override;
        uint32_t GetMaxFramesInFlight() const override;
    private:
        std::vector<ComPtr<ID3D12Fence>> m_fences;
        std::vector<uint64_t> m_fenceValues;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        HANDLE m_fenceEvent = nullptr;
        uint32_t m_maxFramesInFlight = 3;
    };
}
#endif
#endif 
