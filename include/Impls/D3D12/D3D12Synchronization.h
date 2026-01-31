#ifndef CACAO_D3D12SYNCHRONIZATION_H
#define CACAO_D3D12SYNCHRONIZATION_H
#ifdef WIN32
#include "Synchronization.h"
#include "D3D12Common.h"
#include "Device.h"

namespace Cacao
{
    class D3D12Device;
    class D3D12Swapchain;
    class D3D12Queue;

    class CACAO_API D3D12Synchronization : public Synchronization
    {
        uint32_t m_maxFramesInFlight;
        std::vector<ComPtr<ID3D12Fence>> m_fences;
        std::vector<uint64_t> m_fenceValues;
        HANDLE m_fenceEvent = nullptr;
        Ref<D3D12Device> m_d3d12Device;

        friend class D3D12Device;
        friend class D3D12Swapchain;
        friend class D3D12Queue;

        // 供友元类访问的内部方法
        ID3D12Fence* GetFence(uint32_t frameIndex);
        uint64_t GetFenceValue(uint32_t frameIndex) const;
        void IncrementFenceValue(uint32_t frameIndex);

    public:
        static Ref<D3D12Synchronization> Create(const Ref<Device>& device, uint32_t maxFramesInFlight);
        D3D12Synchronization(const Ref<Device>& device, uint32_t maxFramesInFlight);
        void WaitForFrame(uint32_t frameIndex) const override;
        void ResetFrameFence(uint32_t frameIndex) const override;
        uint32_t AcquireNextImageIndex(const Ref<Swapchain>& swapchain, uint32_t frameIndex) const override;
        uint32_t GetMaxFramesInFlight() const override;
        ~D3D12Synchronization() override;
        void WaitIdle() const override;
    };
}
#endif
#endif