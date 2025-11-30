#ifndef CACAO_VKBUFFER_H
#define CACAO_VKBUFFER_H
#include "CacaoBuffer.h"
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
namespace Cacao
{
    class CacaoDevice;
}
namespace Cacao
{
    class CACAO_API VKBuffer final : public CacaoBuffer
    {
    private:
        vk::Buffer m_buffer;
        Ref<CacaoDevice> m_device;
        VmaAllocator m_allocator;
        VmaAllocation m_allocation;
        BufferCreateInfo m_createInfo;
        VmaAllocationInfo m_allocationInfo;
        friend class VKDevice;
        friend class VKCommandBufferEncoder;
        vk::Buffer& GetVulkanBuffer()
        {
            return m_buffer;
        }
    public:
        VKBuffer(const Ref<CacaoDevice>& device, const VmaAllocator& allocator, const BufferCreateInfo& info);
        static Ref<VKBuffer> Create(const Ref<CacaoDevice>& device, const VmaAllocator& allocator,
                                    const BufferCreateInfo& info);
        uint64_t GetSize() const override;
        BufferUsageFlags GetUsage() const override;
        BufferMemoryUsage GetMemoryUsage() const override;
        void* Map() override;
        void Unmap() override;
        void Flush(uint64_t offset, uint64_t size) override;
        uint64_t GetDeviceAddress() const override;
    };
}
;
#endif 
