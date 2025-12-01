#include <utility>
#include "Impls/Vulkan/VKTexture.h"
#include "CacaoDevice.h"
#include "Impls/Vulkan/VKConvert.h"
#include "Impls/Vulkan/VKDevice.h"
namespace Cacao
{
    VKTextureView::VKTextureView(const Ref<CacaoTexture>& texture, const TextureViewDesc& desc) :
        m_desc(desc)
    {
        if (!texture)
        {
            throw std::runtime_error("VKTextureView created with null texture");
        }
        m_texture = std::static_pointer_cast<VKTexture>(texture);
        vk::ImageViewCreateInfo viewInfo = {};
        viewInfo.image = m_texture->GetVulkanImage();
        viewInfo.format = Converter::Convert(desc.FormatOverride);
        switch (desc.ViewType)
        {
        case TextureType::Texture1D: viewInfo.viewType = vk::ImageViewType::e1D;
            break;
        case TextureType::Texture1DArray: viewInfo.viewType = vk::ImageViewType::e1DArray;
            break;
        case TextureType::Texture2D: viewInfo.viewType = vk::ImageViewType::e2D;
            break;
        case TextureType::Texture2DArray: viewInfo.viewType = vk::ImageViewType::e2DArray;
            break;
        case TextureType::Texture3D: viewInfo.viewType = vk::ImageViewType::e3D;
            break;
        case TextureType::TextureCube: viewInfo.viewType = vk::ImageViewType::eCube;
            break;
        case TextureType::TextureCubeArray: viewInfo.viewType = vk::ImageViewType::eCubeArray;
            break;
        }
        vk::ImageAspectFlags aspectFlags;
        if (desc.Aspect & AspectMask::Depth) aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        if (desc.Aspect & AspectMask::Stencil) aspectFlags |= vk::ImageAspectFlagBits::eStencil;
        if (!aspectFlags) aspectFlags = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = desc.BaseMipLevel;
        viewInfo.subresourceRange.levelCount = desc.MipLevelCount;
        viewInfo.subresourceRange.baseArrayLayer = desc.BaseArrayLayer;
        viewInfo.subresourceRange.layerCount = desc.ArrayLayerCount;
        m_imageView = std::static_pointer_cast<VKDevice>(m_texture->m_device)->GetHandle().createImageView(viewInfo);
        if (!m_imageView)
        {
            throw std::runtime_error("Failed to create image view");
        }
    }
    Ref<VKTextureView> VKTextureView::Create(const Ref<CacaoTexture>& texture, const TextureViewDesc& desc)
    {
        return CreateRef<VKTextureView>(texture, desc);
    }
    VKTextureView::VKTextureView(const Ref<CacaoTexture>& texture, const vk::ImageView& view,
                                 TextureViewDesc desc) : m_desc(std::move(desc)), m_imageView(view)
    {
        if (!texture)
        {
            throw std::runtime_error("VKTextureView created with null texture");
        }
        m_texture = std::static_pointer_cast<VKTexture>(texture);
    }
    Ref<VKTextureView> VKTextureView::Create(const Ref<CacaoTexture>& texture, const vk::ImageView& view,
                                             const TextureViewDesc& desc)
    {
        return CreateRef<VKTextureView>(texture, view, desc);
    }
    Ref<CacaoTexture> VKTextureView::GetTexture() const
    {
        return m_texture;
    }
    const TextureViewDesc& VKTextureView::GetDesc() const
    {
        return m_desc;
    }
    VKTexture::VKTexture(const vk::Image& image, const vk::ImageView& imageView, const TextureCreateInfo& info) :
        m_allocator(nullptr),
        m_allocation(nullptr),
        m_allocationInfo()
    {
        m_image = image;
        m_createInfo = info;
        m_imageView = imageView;
    }
    Ref<VKTexture> VKTexture::CreateFromSwapchainImage(const vk::Image& image, const vk::ImageView& imageView,
                                                       const TextureCreateInfo& info)
    {
        auto texture = CreateRef<VKTexture>(image, imageView, info);
        texture->CreateDefaultViewIfNeeded();
        return texture;
    }
    VKTexture::VKTexture(const Ref<CacaoDevice>& device, const VmaAllocator& allocator, const TextureCreateInfo& info)
        : m_device(device), m_allocator(allocator), m_createInfo(info)
    {
        if (!m_device)
        {
            throw std::runtime_error("VKTexture created with null device");
        }
        vk::ImageCreateInfo imageInfo{};
        imageInfo.extent = vk::Extent3D(m_createInfo.Width, m_createInfo.Height, m_createInfo.Depth);
        imageInfo.arrayLayers = m_createInfo.ArrayLayers;
        switch (m_createInfo.Format)
        {
        case Format::RGBA8_UNORM: imageInfo.format = vk::Format::eR8G8B8A8Unorm; break;
        case Format::BGRA8_UNORM: imageInfo.format = vk::Format::eB8G8R8A8Unorm; break;
        case Format::RGBA8_SRGB: imageInfo.format = vk::Format::eR8G8B8A8Srgb; break;
        case Format::BGRA8_SRGB: imageInfo.format = vk::Format::eB8G8R8A8Srgb; break;
        case Format::RGBA16_FLOAT: imageInfo.format = vk::Format::eR16G16B16A16Sfloat; break;
        case Format::RGB10A2_UNORM: imageInfo.format = vk::Format::eA2B10G10R10UnormPack32; break;
        case Format::RGBA32_FLOAT: imageInfo.format = vk::Format::eR32G32B32A32Sfloat; break;
        case Format::R8_UNORM: imageInfo.format = vk::Format::eR8Unorm; break;
        case Format::R16_FLOAT: imageInfo.format = vk::Format::eR16Sfloat; break;
        case Format::D32F: imageInfo.format = vk::Format::eD32Sfloat; break;
        case Format::D24S8: imageInfo.format = vk::Format::eD24UnormS8Uint; break;
        default: throw std::runtime_error("Unsupported texture format in VKTexture");
        }
        switch (m_createInfo.Type)
        {
        case TextureType::Texture1D:
        case TextureType::Texture1DArray:
            imageInfo.imageType = vk::ImageType::e1D;
            break;
        case TextureType::Texture2D:
        case TextureType::Texture2DArray:
            imageInfo.imageType = vk::ImageType::e2D;
            break;
        case TextureType::Texture3D:
            imageInfo.imageType = vk::ImageType::e3D;
            break;
        case TextureType::TextureCube:
        case TextureType::TextureCubeArray:
            imageInfo.imageType = vk::ImageType::e2D;
            imageInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
            break;
        }
        switch (m_createInfo.SampleCount)
        {
        case SampleCount::Count1: imageInfo.samples = vk::SampleCountFlagBits::e1; break;
        case SampleCount::Count2: imageInfo.samples = vk::SampleCountFlagBits::e2; break;
        case SampleCount::Count4: imageInfo.samples = vk::SampleCountFlagBits::e4; break;
        case SampleCount::Count8: imageInfo.samples = vk::SampleCountFlagBits::e8; break;
        case SampleCount::Count16: imageInfo.samples = vk::SampleCountFlagBits::e16; break;
        case SampleCount::Count32: imageInfo.samples = vk::SampleCountFlagBits::e32; break;
        case SampleCount::Count64: imageInfo.samples = vk::SampleCountFlagBits::e64; break;
        default: imageInfo.samples = vk::SampleCountFlagBits::e1; break;
        }
        imageInfo.mipLevels = m_createInfo.MipLevels;
        vk::ImageUsageFlags usageFlags;
        if (m_createInfo.Usage & TextureUsageFlags::TransferSrc) usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        if (m_createInfo.Usage & TextureUsageFlags::TransferDst) usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
        if (m_createInfo.Usage & TextureUsageFlags::Sampled) usageFlags |= vk::ImageUsageFlagBits::eSampled;
        if (m_createInfo.Usage & TextureUsageFlags::Storage) usageFlags |= vk::ImageUsageFlagBits::eStorage;
        if (m_createInfo.Usage & TextureUsageFlags::ColorAttachment) usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::DepthStencilAttachment) usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::InputAttachment) usageFlags |= vk::ImageUsageFlagBits::eInputAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::TransientAttachment) usageFlags |= vk::ImageUsageFlagBits::eTransientAttachment;
        imageInfo.usage = usageFlags;
        switch (m_createInfo.InitialLayout)
        {
        case ImageLayout::ColorAttachment: imageInfo.initialLayout = vk::ImageLayout::eColorAttachmentOptimal; break;
        case ImageLayout::DepthStencilAttachment: imageInfo.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal; break;
        case ImageLayout::DepthStencilReadOnly: imageInfo.initialLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal; break;
        case ImageLayout::General: imageInfo.initialLayout = vk::ImageLayout::eGeneral; break;
        case ImageLayout::ShaderReadOnly: imageInfo.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal; break;
        case ImageLayout::TransferDst: imageInfo.initialLayout = vk::ImageLayout::eTransferDstOptimal; break;
        case ImageLayout::TransferSrc: imageInfo.initialLayout = vk::ImageLayout::eTransferSrcOptimal; break;
        case ImageLayout::Present: imageInfo.initialLayout = vk::ImageLayout::ePresentSrcKHR; break;
        case ImageLayout::Preinitialized: imageInfo.initialLayout = vk::ImageLayout::ePreinitialized; break;
        case ImageLayout::Undefined:
        default: imageInfo.initialLayout = vk::ImageLayout::eUndefined; break;
        }
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VkImageCreateInfo vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
        if (vmaCreateImage(m_allocator, &vkImageInfo, &allocCreateInfo,
                           reinterpret_cast<VkImage*>(&m_image), &m_allocation, &m_allocationInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image in VKTexture");
        }
    }
    VKTexture::~VKTexture()
    {
        if (m_allocator && m_allocation && m_image)
        {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = VK_NULL_HANDLE;
            m_allocation = nullptr;
        }
    }
    Ref<VKTexture> VKTexture::Create(const Ref<CacaoDevice>& device, const VmaAllocator& allocator,
                                     const TextureCreateInfo& info)
    {
        auto texture = CreateRef<VKTexture>(device, allocator, info);
        texture->CreateDefaultViewIfNeeded();
        return texture;
    }
    void VKTexture::CreateDefaultViewIfNeeded()
    {
        if (m_view)
        {
            return;
        }
        TextureViewDesc viewDesc = {};
        viewDesc.ViewType = m_createInfo.Type;
        viewDesc.BaseArrayLayer = 0;
        viewDesc.ArrayLayerCount = m_createInfo.ArrayLayers;
        viewDesc.BaseMipLevel = 0;
        viewDesc.MipLevelCount = m_createInfo.MipLevels;
        viewDesc.FormatOverride = m_createInfo.Format;
        AspectMask aspectFlags = {};
        if (IsDepthFormat(m_createInfo.Format)) aspectFlags |= AspectMask::Depth;
        if (IsStencilFormat(m_createInfo.Format)) aspectFlags |= AspectMask::Stencil;
        if (!aspectFlags) aspectFlags = AspectMask::Color;
        viewDesc.Aspect = aspectFlags;
        if (m_imageView)
        {
            m_view = VKTextureView::Create(shared_from_this(), m_imageView, viewDesc);
            m_imageView = nullptr;
        }
        else
        {
            m_view = VKTextureView::Create(shared_from_this(), viewDesc);
        }
    }
    uint32_t VKTexture::GetWidth() const { return m_createInfo.Width; }
    uint32_t VKTexture::GetHeight() const { return m_createInfo.Height; }
    uint32_t VKTexture::GetDepth() const { return m_createInfo.Depth; }
    uint32_t VKTexture::GetMipLevels() const { return m_createInfo.MipLevels; }
    uint32_t VKTexture::GetArrayLayers() const { return m_createInfo.ArrayLayers; }
    Format VKTexture::GetFormat() const { return m_createInfo.Format; }
    TextureType VKTexture::GetType() const { return m_createInfo.Type; }
    SampleCount VKTexture::GetSampleCount() const { return m_createInfo.SampleCount; }
    TextureUsageFlags VKTexture::GetUsage() const { return m_createInfo.Usage; }
    ImageLayout VKTexture::GetCurrentLayout() const { return m_createInfo.InitialLayout; }
    Ref<CacaoTextureView> VKTexture::CreateView(const TextureViewDesc& desc)
    {
        return VKTextureView::Create(shared_from_this(), desc);
    }
}
