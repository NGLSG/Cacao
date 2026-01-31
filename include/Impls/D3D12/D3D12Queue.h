#ifndef CACAO_D3D12QUEUE_H
#define CACAO_D3D12QUEUE_H
#ifdef WIN32
#include "Queue.h"
#include "D3D12Common.h"
#include <mutex>

namespace Cacao
{
    class D3D12Device;
    class D3D12Swapchain;
    class D3D12Synchronization;

    class CACAO_API D3D12Queue final : public Queue
    {
        Ref<D3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        QueueType m_type;
        uint32_t m_index;
        std::mutex m_submitMutex;

        // 用于 WaitIdle 的内部 fence
        ComPtr<ID3D12Fence> m_idleFence;
        uint64_t m_idleFenceValue = 0;
        HANDLE m_idleFenceEvent = nullptr;

        friend class D3D12Swapchain;

        ID3D12CommandQueue* GetD3D12CommandQueue() const
        {
            return m_commandQueue.Get();
        }
        
        ID3D12CommandQueue* GetCommandQueue() const
        {
            return m_commandQueue.Get();
        }

    public:
        static Ref<D3D12Queue> Create(const Ref<Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue,
                                      QueueType type, uint32_t index);
        D3D12Queue(const Ref<Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue, QueueType type,
                   uint32_t index);
        ~D3D12Queue() override;

        QueueType GetType() const override;
        uint32_t GetIndex() const override;
        void Submit(const Ref<CommandBufferEncoder>& cmd, const Ref<Synchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(std::span<const Ref<CommandBufferEncoder>> cmds, const Ref<Synchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(const Ref<CommandBufferEncoder>& cmd) override;
        void WaitIdle() override;
    };
}
#endif
#endif