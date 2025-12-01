#ifndef CACAO_VKTEXTURE_H
#define CACAO_VKTEXTURE_H
#include "CacaoTexture.h"
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
namespace Cacao
{
    class VKTexture;
    class CacaoDevice;
    class CACAO_API VKTextureView : public CacaoTextureView
    {
    private:
        Ref<VKTexture> m_texture;
        TextureViewDesc m_desc;
        vk::ImageView m_imageView;
        friend class VKCommandBufferEncoder;
        friend class VKDescriptorSet;
    public:
        VKTextureView(const Ref<CacaoTexture>& texture, const TextureViewDesc& desc);
        static Ref<VKTextureView> Create(const Ref<CacaoTexture>& texture, const TextureViewDesc& desc);
        VKTextureView(const Ref<CacaoTexture>& texture, const vk::ImageView& view, TextureViewDesc desc);
        static Ref<VKTextureView> Create(const Ref<CacaoTexture>& texture, const vk::ImageView& view,
                                         const TextureViewDesc& desc);
        Ref<CacaoTexture> GetTexture() const override;
        const TextureViewDesc& GetDesc() const override;
        vk::ImageView& GetHandle()
        {
            return m_imageView;
        }
    };
    class CACAO_API VKTexture final : public CacaoTexture
    {
        friend class VKSwapchain;
        friend class VKTextureView;
        friend class VKCommandBufferEncoder;
        friend class VKDescriptorSet;
        vk::Image m_image;
        vk::Image& GetVulkanImage() { return m_image; }
        vk::ImageView m_imageView;
        Ref<VKTextureView> m_view;
        VmaAllocator m_allocator;
        VmaAllocation m_allocation;
        VmaAllocationInfo m_allocationInfo;
        Ref<CacaoDevice> m_device;
        TextureCreateInfo m_createInfo;
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
        Ref<CacaoTextureView> CreateView(const TextureViewDesc& desc) override;
        ~VKTexture() override;
        Ref<CacaoTextureView> GetDefaultView() override
        {
            return m_view;
        }
        void CreateDefaultViewIfNeeded() override;
    };
}
#endif
