#ifndef CACAO_D3D12QUEUE_H
#define CACAO_D3D12QUEUE_H
#ifdef WIN32
#include "Queue.h"
#include  "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Queue : public Queue
    {
    public:
        QueueType GetType() const override;
        uint32_t GetIndex() const override;
        void Submit(const Ref<CommandBufferEncoder>& cmd, const Ref<Synchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(std::span<const Ref<CommandBufferEncoder>> cmds, const Ref<Synchronization>& sync,
                    uint32_t frameIndex) override;
        void Submit(const Ref<CommandBufferEncoder>& cmd) override;
        void WaitIdle() const override;
    private:
        ComPtr<ID3D12CommandQueue> m_commandQueue;
    };
}
#endif
#endif 
