#ifdef WIN32
#include "Impls/D3D12/D3D12DescriptorPool.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12DescriptorSet.h"
#include <stdexcept>

namespace Cacao
{
    D3D12DescriptorPool::D3D12DescriptorPool(const Ref<Device>& device, const DescriptorPoolCreateInfo& info)
        : m_createInfo(info)
    {
        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device) throw std::runtime_error("Invalid device");

        // 统计总需求
        for (const auto& size : info.PoolSizes)
        {
            if (size.Type == DescriptorType::Sampler)
                m_samplerCapacity += size.Count;
            else
                m_cbvSrvUavCapacity += size.Count;
        }

        // 创建 CBV/SRV/UAV 堆
        if (m_cbvSrvUavCapacity > 0)
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = m_cbvSrvUavCapacity;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 必须可见

            if (FAILED(m_device->GetHandle()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap))))
                throw std::runtime_error("Failed to create CBV/SRV/UAV heap");

            m_cbvSrvUavIncrement = m_device->GetHandle()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        // 创建 Sampler 堆
        if (m_samplerCapacity > 0)
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = m_samplerCapacity;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 必须可见

            if (FAILED(m_device->GetHandle()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_samplerHeap))))
                throw std::runtime_error("Failed to create Sampler heap");

            m_samplerIncrement = m_device->GetHandle()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::AllocateCbvSrvUav(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle)
    {
        if (m_cbvSrvUavOffset + count > m_cbvSrvUavCapacity)
            throw std::runtime_error("Descriptor pool exhausted (CBV/SRV/UAV)");

        auto cpuStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
        auto gpuStart = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

        outCpuHandle = cpuStart;
        outCpuHandle.ptr += m_cbvSrvUavOffset * m_cbvSrvUavIncrement;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = gpuStart;
        gpuHandle.ptr += m_cbvSrvUavOffset * m_cbvSrvUavIncrement;

        m_cbvSrvUavOffset += count;
        return gpuHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::AllocateSampler(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle)
    {
        if (m_samplerOffset + count > m_samplerCapacity)
            throw std::runtime_error("Descriptor pool exhausted (Sampler)");

        auto cpuStart = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
        auto gpuStart = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();

        outCpuHandle = cpuStart;
        outCpuHandle.ptr += m_samplerOffset * m_samplerIncrement;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = gpuStart;
        gpuHandle.ptr += m_samplerOffset * m_samplerIncrement;

        m_samplerOffset += count;
        return gpuHandle;
    }

    Ref<DescriptorSet> D3D12DescriptorPool::AllocateDescriptorSet(const Ref<DescriptorSetLayout>& layout)
    {
        return D3D12DescriptorSet::Create(m_device, std::dynamic_pointer_cast<D3D12DescriptorPool>(shared_from_this()), layout);
    }

    void D3D12DescriptorPool::Reset()
    {
        m_cbvSrvUavOffset = 0;
        m_samplerOffset = 0;
    }

    Ref<D3D12DescriptorPool> D3D12DescriptorPool::Create(const Ref<Device>& device, const DescriptorPoolCreateInfo& info)
    {
        return CreateRef<D3D12DescriptorPool>(device, info);
    }
}
#endif