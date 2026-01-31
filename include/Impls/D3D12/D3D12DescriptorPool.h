#ifndef CACAO_D3D12DESCRIPTORPOOL_H
#define CACAO_D3D12DESCRIPTORPOOL_H
#ifdef WIN32
#include "DescriptorPool.h"
#include "D3D12Common.h"

namespace Cacao
{
    class D3D12Device;

    class CACAO_API D3D12DescriptorPool : public DescriptorPool
    {
        Ref<D3D12Device> m_device;
        DescriptorPoolCreateInfo m_createInfo;

        // D3D12 描述符堆
        ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
        ComPtr<ID3D12DescriptorHeap> m_samplerHeap;

        // 简单的线性分配器跟踪
        uint32_t m_cbvSrvUavOffset = 0;
        uint32_t m_samplerOffset = 0;
        uint32_t m_cbvSrvUavCapacity = 0;
        uint32_t m_samplerCapacity = 0;

        uint32_t m_cbvSrvUavIncrement = 0;
        uint32_t m_samplerIncrement = 0;

        friend class D3D12DescriptorSet;

    public:
        D3D12DescriptorPool(const Ref<Device>& device, const DescriptorPoolCreateInfo& info);
        static Ref<D3D12DescriptorPool> Create(const Ref<Device>& device, const DescriptorPoolCreateInfo& info);

        Ref<DescriptorSet> AllocateDescriptorSet(const Ref<DescriptorSetLayout>& layout) override;
        void Reset() override;

        // 内部分配方法
        D3D12_GPU_DESCRIPTOR_HANDLE AllocateCbvSrvUav(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE AllocateSampler(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle);

        // [新增] 获取堆指针，供 CommandBufferEncoder 绑定使用
        ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_cbvSrvUavHeap.Get(); }
        ID3D12DescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.Get(); }

        ~D3D12DescriptorPool() override = default;
    };
}
#endif
#endif