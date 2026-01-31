#ifndef CACAO_D3D12DESCRIPTORSET_H
#define CACAO_D3D12DESCRIPTORSET_H
#ifdef WIN32
#include "DescriptorSet.h"
#include "D3D12Common.h"
#include "Impls/D3D12/D3D12DescriptorPool.h" // 需要包含 Pool 定义以访问 GetHeap

namespace Cacao
{
    class D3D12Device;
    class D3D12DescriptorSetLayout;

    class CACAO_API D3D12DescriptorSet : public DescriptorSet
    {
        Ref<D3D12Device> m_device;
        Ref<D3D12DescriptorPool> m_pool;
        Ref<D3D12DescriptorSetLayout> m_layout;

        // GPU 描述符句柄
        D3D12_GPU_DESCRIPTOR_HANDLE m_cbvSrvUavGpuHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE m_samplerGpuHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_cbvSrvUavCpuHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_samplerCpuHandle = {};

        uint32_t m_cbvSrvUavCount = 0;
        uint32_t m_samplerCount = 0;

        friend class D3D12CommandBufferEncoder;

    public:
        D3D12DescriptorSet(const Ref<D3D12Device>& device,
                           const Ref<D3D12DescriptorPool>& pool,
                           const Ref<DescriptorSetLayout>& layout);
        ~D3D12DescriptorSet() override = default;

        void WriteBuffer(const BufferWriteInfo& info) override;
        void WriteBuffers(const BufferWriteInfos& infos) override;
        void WriteTexture(const TextureWriteInfo& info) override;
        void WriteTextures(const TextureWriteInfos& infos) override;
        void WriteSampler(const SamplerWriteInfo& info) override;
        void WriteSamplers(const SamplerWriteInfos& infos) override;
        void WriteAccelerationStructure(const AccelerationStructureWriteInfo& info) override;
        void WriteAccelerationStructures(const AccelerationStructureWriteInfos& infos) override;
        void Update() override;

        D3D12_GPU_DESCRIPTOR_HANDLE GetCbvSrvUavGpuHandle() const { return m_cbvSrvUavGpuHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGpuHandle() const { return m_samplerGpuHandle; }

        // [新增] 获取关联的堆
        ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_pool->GetCbvSrvUavHeap(); }
        ID3D12DescriptorHeap* GetSamplerHeap() const { return m_pool->GetSamplerHeap(); }

        static Ref<DescriptorSet> Create(const Ref<D3D12Device>& device,
                                          const Ref<D3D12DescriptorPool>& pool,
                                          const Ref<DescriptorSetLayout>& layout);
    };
}
#endif
#endif