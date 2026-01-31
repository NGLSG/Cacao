#ifdef WIN32
#include "Impls/D3D12/D3D12Synchronization.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Swapchain.h"

namespace Cacao
{
    ID3D12Fence* D3D12Synchronization::GetFence(uint32_t frameIndex)
    {
        return m_fences[frameIndex].Get();
    }

    uint64_t D3D12Synchronization::GetFenceValue(uint32_t frameIndex) const
    {
        return m_fenceValues[frameIndex];
    }

    void D3D12Synchronization::IncrementFenceValue(uint32_t frameIndex)
    {
        ++m_fenceValues[frameIndex];
    }

    Ref<D3D12Synchronization> D3D12Synchronization::Create(const Ref<Device>& device, uint32_t maxFramesInFlight)
    {
        return CreateRef<D3D12Synchronization>(device, maxFramesInFlight);
    }

    D3D12Synchronization::D3D12Synchronization(const Ref<Device>& device, uint32_t maxFramesInFlight) :
        m_maxFramesInFlight(maxFramesInFlight)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12Synchronization created with null device");
        }

        m_d3d12Device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_d3d12Device)
        {
            throw std::runtime_error("D3D12Synchronization requires a D3D12Device");
        }

        m_fences.resize(maxFramesInFlight);
        m_fenceValues.resize(maxFramesInFlight, 0);

        // 创建 fence 事件句柄
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            throw std::runtime_error("Failed to create fence event");
        }

        // 为每一帧创建 fence
        for (uint32_t i = 0; i < maxFramesInFlight; ++i)
        {
            HRESULT hr = m_d3d12Device->GetHandle()->CreateFence(
                0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&m_fences[i])
            );

            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to create D3D12 fence");
            }

            // 初始化 fence 值
            m_fenceValues[i] = 0;
        }
    }

    void D3D12Synchronization::WaitForFrame(uint32_t frameIndex) const
    {
        ID3D12Fence* fence = m_fences[frameIndex].Get();
        uint64_t fenceValue = m_fenceValues[frameIndex];

        // 检查 fence 是否已经完成
        if (fence->GetCompletedValue() < fenceValue)
        {
            // 设置事件，当 fence 达到指定值时触发
            HRESULT hr = fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to set fence event on completion");
            }

            // 等待事件触发
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void D3D12Synchronization::ResetFrameFence(uint32_t frameIndex) const
    {
        // D3D12 的 fence 不需要像 Vulkan 那样显式重置
        // fence 值会在每次信号时递增，所以这里可以留空
        // 或者可以在这里递增 fence 值以准备下一次使用
        // 注意: 实际的信号操作通常在命令队列提交后进行
    }

    uint32_t D3D12Synchronization::AcquireNextImageIndex(const Ref<Swapchain>& swapchain, uint32_t frameIndex) const
    {
        if (! swapchain)
        {
            throw std::runtime_error("AcquireNextImageIndex called with null swapchain");
        }

        auto d3d12Swapchain = std::dynamic_pointer_cast<D3D12Swapchain>(swapchain);
        if (!d3d12Swapchain)
        {
            throw std::runtime_error("AcquireNextImageIndex requires a D3D12Swapchain");
        }

        // D3D12 使用 GetCurrentBackBufferIndex 获取当前后缓冲区索引
        // 注意: D3D12 不需要像 Vulkan 那样使用信号量来同步图像获取
        return d3d12Swapchain->GetHandle()->GetCurrentBackBufferIndex();
    }

    uint32_t D3D12Synchronization::GetMaxFramesInFlight() const
    {
        return m_maxFramesInFlight;
    }

    D3D12Synchronization::~D3D12Synchronization()
    {
        // 在销毁前等待所有帧完成
        WaitIdle();

        // 关闭事件句柄
        if (m_fenceEvent != nullptr)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        // ComPtr 会自动释放 fence 资源
        m_fences.clear();
    }

    void D3D12Synchronization::WaitIdle() const
    {
        // 等待所有帧的 fence 完成
        for (uint32_t i = 0; i < m_maxFramesInFlight; ++i)
        {
            WaitForFrame(i);
        }
    }
}
#endif