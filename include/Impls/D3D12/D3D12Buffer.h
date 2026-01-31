#ifndef CACAO_D3D12BUFFER_H
#define CACAO_D3D12BUFFER_H
#ifdef WIN32
#include "Buffer.h"
#include "D3D12Common.h"
#include "D3D12MemAlloc.h"
#include "Device.h"

namespace Cacao
{
    class D3D12Device;

    class CACAO_API D3D12Buffer final : public Buffer
    {
    private:
        ComPtr<ID3D12Resource> m_resource;
        Ref<D3D12Device> m_device;
        D3D12MA::Allocator* m_allocator;
        D3D12MA::Allocation* m_allocation;
        BufferCreateInfo m_createInfo;
        void* m_mappedData = nullptr;
        bool m_isPersistentlyMapped = false;

        friend class D3D12Device;
        friend class D3D12CommandBufferEncoder;
        friend class D3D12DescriptorSet;

    public:
        D3D12Buffer(const Ref<Device>& device, D3D12MA::Allocator* allocator, const BufferCreateInfo& info);
        static Ref<D3D12Buffer> Create(const Ref<Device>& device, D3D12MA::Allocator* allocator,
                                       const BufferCreateInfo& info);

        uint64_t GetSize() const override;
        BufferUsageFlags GetUsage() const override;
        BufferMemoryUsage GetMemoryUsage() const override;
        void* Map() override;
        void Unmap() override;
        void Flush(uint64_t offset, uint64_t size) override;
        uint64_t GetDeviceAddress() const override;

        ID3D12Resource* GetHandle() const
        {
            return m_resource.Get();
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
        {
            return m_resource->GetGPUVirtualAddress();
        }

        ~D3D12Buffer();
    };
}
#endif
#endif