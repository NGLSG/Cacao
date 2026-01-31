#ifdef WIN32
#include "Impls/D3D12/D3D12Buffer.h"
#include "Impls/D3D12/D3D12Device.h"
#include <stdexcept>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace Cacao
{
    D3D12Buffer::D3D12Buffer(const Ref<Device>& device, D3D12MA::Allocator* allocator,
                             const BufferCreateInfo& info) :
        m_allocator(allocator), m_createInfo(info), m_allocation(nullptr)
    {
        if (!device || !allocator) throw std::runtime_error("Invalid D3D12Buffer arguments");
        if (info.Size == 0) throw std::runtime_error("Buffer size cannot be zero");

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

        // 根据用途选择堆类型
        switch (info.MemoryUsage)
        {
        case BufferMemoryUsage::GpuOnly:
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            initialState = D3D12_RESOURCE_STATE_COMMON;
            break;
        case BufferMemoryUsage::CpuToGpu: // Upload Heap
            allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ; // Upload 堆初始状态必须是 GenericRead
            break;
        case BufferMemoryUsage::GpuToCpu: // Readback Heap
            allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
            initialState = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        default:
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            break;
        }

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = info.Size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if ((info.Usage & BufferUsageFlags::StorageBuffer) && allocationDesc.HeapType == D3D12_HEAP_TYPE_DEFAULT)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        HRESULT hr = m_allocator->CreateResource(
            &allocationDesc, &resourceDesc, initialState, nullptr,
            &m_allocation, IID_PPV_ARGS(&m_resource));

        if (FAILED(hr)) throw std::runtime_error("Failed to create D3D12 buffer");
    }

    Ref<D3D12Buffer> D3D12Buffer::Create(const Ref<Device>& device, D3D12MA::Allocator* allocator, const BufferCreateInfo& info)
    {
        return CreateRef<D3D12Buffer>(device, allocator, info);
    }

    void* D3D12Buffer::Map()
    {
        // [RHI 修正] 支持持久映射。如果已经映射，直接返回旧指针。
        if (m_mappedData) return m_mappedData;

        if (m_createInfo.MemoryUsage == BufferMemoryUsage::GpuOnly)
            throw std::runtime_error("Cannot map GPU-only buffer");

        // 对于 Upload Heap，读取范围设为 0 以避免 CPU Cache Miss 开销
        D3D12_RANGE readRange = { 0, 0 };
        if (m_createInfo.MemoryUsage == BufferMemoryUsage::GpuToCpu)
        {
            readRange = { 0, static_cast<SIZE_T>(m_createInfo.Size) };
        }

        HRESULT hr = m_resource->Map(0, &readRange, &m_mappedData);
        if (FAILED(hr)) throw std::runtime_error("Failed to map D3D12 buffer");

        return m_mappedData;
    }

    void D3D12Buffer::Unmap()
    {
        // [RHI 修正] 为了与 Vulkan 行为一致（需要显式调用 Unmap），这里我们保留 Unmap 逻辑。
        // 但在 RHI 使用模式中，如果用户做了 Persistent Map，就不应该在每帧调用 Unmap。
        // 为了安全起见，这里执行真实的 Unmap，但 main.cpp 不应调用它。
        if (m_mappedData)
        {
            D3D12_RANGE writtenRange = { 0, 0 };
            if (m_createInfo.MemoryUsage == BufferMemoryUsage::CpuToGpu)
                writtenRange = { 0, static_cast<SIZE_T>(m_createInfo.Size) }; // 告诉 GPU 我们写了多少

            m_resource->Unmap(0, &writtenRange);
            m_mappedData = nullptr;
        }
    }

    void D3D12Buffer::Flush(uint64_t offset, uint64_t size)
    {
        // [核心修复]
        // 绝对不要在这里调用 Unmap/Map！这会改变虚拟地址，导致外部持有的指针失效。
        // D3D12 UPLOAD 堆是 Coherent 的，写入即对 GPU 可见。
        // 这里只需要一个内存屏障防止编译器乱序。

        if (m_createInfo.MemoryUsage == BufferMemoryUsage::CpuToGpu)
        {
            std::atomic_thread_fence(std::memory_order_release);
        }
    }

    D3D12Buffer::~D3D12Buffer()
    {
        if (m_mappedData) m_resource->Unmap(0, nullptr);
        if (m_allocation) m_allocation->Release();
    }

    uint64_t D3D12Buffer::GetSize() const { return m_createInfo.Size; }
    BufferUsageFlags D3D12Buffer::GetUsage() const { return m_createInfo.Usage; }
    BufferMemoryUsage D3D12Buffer::GetMemoryUsage() const { return m_createInfo.MemoryUsage; }
    uint64_t D3D12Buffer::GetDeviceAddress() const { return m_resource->GetGPUVirtualAddress(); }
}
#endif