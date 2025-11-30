#ifndef CACAO_VKTEXTURE_H
#define CACAO_VKTEXTURE_H
#include "CacaoTexture.h"
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
namespace Cacao
{
    class CacaoDevice;
    class CACAO_API VKTexture final : public CacaoTexture
    {
        vk::Image m_image;
        vk::ImageView m_imageView;
        friend class VKCommandBufferEncoder;
        vk::ImageView& GetVulkanImageView() { return m_imageView; }
        vk::Image& GetVulkanImage() { return m_image; }
        VmaAllocator m_allocator;
        VmaAllocation m_allocation;
        VmaAllocationInfo m_allocationInfo;
        Ref<CacaoDevice> m_device;
        TextureCreateInfo m_createInfo;
        friend class VKSwapchain;
    public:
        VKTexture(const vk::Image& image, const vk::ImageView& imageView, const TextureCreateInfo& info);
        static Ref<VKTexture> CreateFromSwapchainImage(const vk::Image& image, const vk::ImageView& imageView,
                                                       const TextureCreateInfo& info);
        VKTexture(const Ref<CacaoDevice>& device, const VmaAllocator& allocator, const TextureCreateInfo& info);
        static Ref<VKTexture> Create(const Ref<CacaoDevice>& device, const VmaAllocator& allocator,
                                     const TextureCreateInfo& info);
        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        uint32_t GetDepth() const override;
        uint32_t GetMipLevels() const override;
        uint32_t GetArrayLayers() const override;
        Format GetFormat() const override;
        TextureType GetType() const override;
        SampleCount GetSampleCount() const override;
        TextureUsageFlags GetUsage() const override;
        ImageLayout GetCurrentLayout() const override;
    };
}
#endif 
