#ifdef WIN32
#include "Impls/D3D12/D3D12Queue.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Synchronization.h"
#include "Impls/D3D12/D3D12CommandBufferEncoder.h"
#include <mutex>

namespace Cacao
{
    Ref<D3D12Queue> D3D12Queue::Create(const Ref<Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue,
                                       QueueType type, uint32_t index)
    {
        return CreateRef<D3D12Queue>(device, commandQueue, type, index);
    }

    D3D12Queue::D3D12Queue(const Ref<Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue, QueueType type,
                           uint32_t index) :
        m_type(type), m_index(index)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12Queue created with null device");
        }
        if (!commandQueue)
        {
            throw std::runtime_error("D3D12Queue created with null command queue");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12Queue requires a D3D12Device");
        }

        m_commandQueue = commandQueue;

        // 创建用于 WaitIdle 的内部 fence
        HRESULT hr = m_device->GetHandle()->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_idleFence)
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create idle fence for D3D12Queue");
        }

        m_idleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_idleFenceEvent == nullptr)
        {
            throw std::runtime_error("Failed to create fence event for D3D12Queue");
        }
    }

    D3D12Queue::~D3D12Queue()
    {
        // 确保队列完成所有工作
        WaitIdle();

        if (m_idleFenceEvent != nullptr)
        {
            CloseHandle(m_idleFenceEvent);
            m_idleFenceEvent = nullptr;
        }
    }

    QueueType D3D12Queue::GetType() const
    {
        return m_type;
    }

    uint32_t D3D12Queue::GetIndex() const
    {
        return m_index;
    }

    void D3D12Queue::Submit(const Ref<CommandBufferEncoder>& cmd, const Ref<Synchronization>& sync,
                            uint32_t frameIndex)
    {
        if (!cmd)
        {
            throw std::runtime_error("D3D12Queue::Submit called with null command buffer");
        }

        auto* d3d12Cmd = static_cast<D3D12CommandBufferEncoder*>(cmd.get());
        auto* d3d12Sync = static_cast<D3D12Synchronization*>(sync.get());

        // 获取命令列表
        ID3D12CommandList* cmdList = d3d12Cmd->GetCommandList();
        if (!cmdList)
        {
            throw std::runtime_error("D3D12Queue::Submit called with invalid command list");
        }

        {
            std::lock_guard<std::mutex> lock(m_submitMutex);

            // 执行命令列表
            ID3D12CommandList* cmdLists[] = {cmdList};
            m_commandQueue->ExecuteCommandLists(1, cmdLists);

            // 如果提供了同步对象，发出信号
            if (d3d12Sync)
            {
                // 递增 fence 值并发出信号
                d3d12Sync->IncrementFenceValue(frameIndex);
                uint64_t fenceValue = d3d12Sync->GetFenceValue(frameIndex);
                ID3D12Fence* fence = d3d12Sync->GetFence(frameIndex);

                HRESULT hr = m_commandQueue->Signal(fence, fenceValue);
                if (FAILED(hr))
                {
                    throw std::runtime_error("Failed to signal fence after queue submit");
                }
            }
        }
    }

    void D3D12Queue::Submit(std::span<const Ref<CommandBufferEncoder>> cmds, const Ref<Synchronization>& sync,
                            uint32_t frameIndex)
    {
        if (cmds.empty())
        {
            return;
        }

        auto* d3d12Sync = static_cast<D3D12Synchronization*>(sync.get());

        // 收集所有命令列表
        std::vector<ID3D12CommandList*> cmdLists;
        cmdLists.reserve(cmds.size());

        for (const auto& cmd : cmds)
        {
            if (!cmd)
            {
                throw std::runtime_error("D3D12Queue::Submit called with null command buffer in array");
            }

            auto* d3d12Cmd = static_cast<D3D12CommandBufferEncoder*>(cmd.get());
            ID3D12CommandList* cmdList = d3d12Cmd->GetCommandList();

            if (!cmdList)
            {
                throw std::runtime_error("D3D12Queue::Submit called with invalid command list in array");
            }

            cmdLists.push_back(cmdList);
        }

        {
            std::lock_guard<std::mutex> lock(m_submitMutex);

            // 批量执行命令列表
            m_commandQueue->ExecuteCommandLists(
                static_cast<UINT>(cmdLists.size()),
                cmdLists.data()
            );

            // 如果提供了同步对象，发出信号
            if (d3d12Sync)
            {
                d3d12Sync->IncrementFenceValue(frameIndex);
                uint64_t fenceValue = d3d12Sync->GetFenceValue(frameIndex);
                ID3D12Fence* fence = d3d12Sync->GetFence(frameIndex);

                HRESULT hr = m_commandQueue->Signal(fence, fenceValue);
                if (FAILED(hr))
                {
                    throw std::runtime_error("Failed to signal fence after batch queue submit");
                }
            }
        }
    }

    void D3D12Queue::Submit(const Ref<CommandBufferEncoder>& cmd)
    {
        if (!cmd)
        {
            throw std::runtime_error("D3D12Queue::Submit called with null command buffer");
        }

        auto* d3d12Cmd = static_cast<D3D12CommandBufferEncoder*>(cmd.get());
        ID3D12CommandList* cmdList = d3d12Cmd->GetCommandList();

        if (!cmdList)
        {
            throw std::runtime_error("D3D12Queue::Submit called with invalid command list");
        }

        {
            std::lock_guard<std::mutex> lock(m_submitMutex);

            ID3D12CommandList* cmdLists[] = {cmdList};
            m_commandQueue->ExecuteCommandLists(1, cmdLists);
        }
    }

    void D3D12Queue::WaitIdle()
    {

        std::lock_guard<std::mutex> lock(m_submitMutex);

        // 递增 fence 值
        m_idleFenceValue++;
        uint64_t currentFenceValue = m_idleFenceValue;

        // 发出信号
        HRESULT hr = m_commandQueue->Signal(m_idleFence.Get(), currentFenceValue);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to signal idle fence");
        }

        // 等待 fence 完成
        if (m_idleFence->GetCompletedValue() < currentFenceValue)
        {
            hr = m_idleFence->SetEventOnCompletion(currentFenceValue, m_idleFenceEvent);
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to set event on completion for idle fence");
            }

            WaitForSingleObject(m_idleFenceEvent, INFINITE);
        }
    }
}
#endif
