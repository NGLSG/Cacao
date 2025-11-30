#include "Impls/Vulkan/VKTexture.h"
#include "CacaoDevice.h"
#include "Impls/Vulkan/VKDevice.h"
namespace Cacao
{
    VKTexture::VKTexture(const vk::Image& image, const vk::ImageView& imageView, const TextureCreateInfo& info) :
        m_allocator(nullptr),
        m_allocation(nullptr),
        m_allocationInfo()
    {
        m_image = image;
        m_imageView = imageView;
        m_createInfo = info;
    }
    Ref<VKTexture> VKTexture::CreateFromSwapchainImage(const vk::Image& image, const vk::ImageView& imageView,
                                                       const TextureCreateInfo& info)
    {
        return CreateRef<VKTexture>(image, imageView, info);
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
        case Format::RGBA8_UNORM: imageInfo.format = vk::Format::eR8G8B8A8Unorm;
            break;
        case Format::BGRA8_UNORM: imageInfo.format = vk::Format::eB8G8R8A8Unorm;
            break;
        case Format::RGBA8_SRGB: imageInfo.format = vk::Format::eR8G8B8A8Srgb;
            break;
        case Format::BGRA8_SRGB: imageInfo.format = vk::Format::eB8G8R8A8Srgb;
            break;
        case Format::RGBA16_FLOAT: imageInfo.format = vk::Format::eR16G16B16A16Sfloat;
            break;
        case Format::RGB10A2_UNORM: imageInfo.format = vk::Format::eA2B10G10R10UnormPack32;
            break;
        case Format::RGBA32_FLOAT: imageInfo.format = vk::Format::eR32G32B32A32Sfloat;
            break;
        case Format::R8_UNORM: imageInfo.format = vk::Format::eR8Unorm;
            break;
        case Format::R16_FLOAT: imageInfo.format = vk::Format::eR16Sfloat;
            break;
        case Format::D32F: imageInfo.format = vk::Format::eD32Sfloat;
            break;
        case Format::D24S8: imageInfo.format = vk::Format::eD24UnormS8Uint;
            break;
        default:
            throw std::runtime_error("Unsupported texture format in VKTexture");
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
        case SampleCount::Count1: imageInfo.samples = vk::SampleCountFlagBits::e1;
            break;
        case SampleCount::Count2: imageInfo.samples = vk::SampleCountFlagBits::e2;
            break;
        case SampleCount::Count4: imageInfo.samples = vk::SampleCountFlagBits::e4;
            break;
        case SampleCount::Count8: imageInfo.samples = vk::SampleCountFlagBits::e8;
            break;
        case SampleCount::Count16: imageInfo.samples = vk::SampleCountFlagBits::e16;
            break;
        case SampleCount::Count32: imageInfo.samples = vk::SampleCountFlagBits::e32;
            break;
        case SampleCount::Count64: imageInfo.samples = vk::SampleCountFlagBits::e64;
            break;
        default: imageInfo.samples = vk::SampleCountFlagBits::e1;
            break;
        }
        imageInfo.mipLevels = m_createInfo.MipLevels;
        vk::ImageUsageFlags usageFlags;
        if (m_createInfo.Usage & TextureUsageFlags::TransferSrc) usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        if (m_createInfo.Usage & TextureUsageFlags::TransferDst) usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
        if (m_createInfo.Usage & TextureUsageFlags::Sampled) usageFlags |= vk::ImageUsageFlagBits::eSampled;
        if (m_createInfo.Usage & TextureUsageFlags::Storage) usageFlags |= vk::ImageUsageFlagBits::eStorage;
        if (m_createInfo.Usage & TextureUsageFlags::ColorAttachment)
            usageFlags |=
                vk::ImageUsageFlagBits::eColorAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::DepthStencilAttachment)
            usageFlags |=
                vk::ImageUsageFlagBits::eDepthStencilAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::InputAttachment)
            usageFlags |=
                vk::ImageUsageFlagBits::eInputAttachment;
        if (m_createInfo.Usage & TextureUsageFlags::TransientAttachment)
            usageFlags |=
                vk::ImageUsageFlagBits::eTransientAttachment;
        imageInfo.usage = usageFlags;
        switch (m_createInfo.InitialLayout)
        {
        case ImageLayout::ColorAttachment: imageInfo.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
            break;
        case ImageLayout::DepthStencilAttachment: imageInfo.initialLayout =
                vk::ImageLayout::eDepthStencilAttachmentOptimal;
            break;
        case ImageLayout::DepthStencilReadOnly: imageInfo.initialLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            break;
        case ImageLayout::General: imageInfo.initialLayout = vk::ImageLayout::eGeneral;
            break;
        case ImageLayout::ShaderReadOnly: imageInfo.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            break;
        case ImageLayout::TransferDst: imageInfo.initialLayout = vk::ImageLayout::eTransferDstOptimal;
            break;
        case ImageLayout::TransferSrc: imageInfo.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
            break;
        case ImageLayout::Present: imageInfo.initialLayout = vk::ImageLayout::ePresentSrcKHR;
            break;
        case ImageLayout::Preinitialized: imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
            break;
        case ImageLayout::Undefined:
        default: imageInfo.initialLayout = vk::ImageLayout::eUndefined;
            break;
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
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = m_image;
        viewInfo.format = imageInfo.format;
        switch (m_createInfo.Type)
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
        if (IsDepthFormat(m_createInfo.Format)) aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        if (IsStencilFormat(m_createInfo.Format)) aspectFlags |= vk::ImageAspectFlagBits::eStencil;
        if (!aspectFlags) aspectFlags = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_createInfo.MipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = m_createInfo.ArrayLayers;
        m_imageView = std::static_pointer_cast<VKDevice>(m_device)->GetVulkanDevice().createImageView(viewInfo);
        if (!m_imageView)
        {
            throw std::runtime_error("Failed to create image view in VKTexture");
        }
    }
    uint32_t VKTexture::GetWidth() const
    {
        return m_createInfo.Width;
    }
    uint32_t VKTexture::GetHeight() const
    {
        return m_createInfo.Height;
    }
    uint32_t VKTexture::GetDepth() const
    {
        return m_createInfo.Depth;
    }
    uint32_t VKTexture::GetMipLevels() const
    {
        return m_createInfo.MipLevels;
    }
    uint32_t VKTexture::GetArrayLayers() const
    {
        return m_createInfo.ArrayLayers;
    }
    Format VKTexture::GetFormat() const
    {
        return m_createInfo.Format;
    }
    TextureType VKTexture::GetType() const
    {
        return m_createInfo.Type;
    }
    SampleCount VKTexture::GetSampleCount() const
    {
        return m_createInfo.SampleCount;
    }
    TextureUsageFlags VKTexture::GetUsage() const
    {
        return m_createInfo.Usage;
    }
    ImageLayout VKTexture::GetCurrentLayout() const
    {
        return m_createInfo.InitialLayout;
    }
    Ref<VKTexture> VKTexture::Create(const Ref<CacaoDevice>& device, const VmaAllocator& allocator,
                                     const TextureCreateInfo& info)
    {
        auto texture = CreateRef<VKTexture>(device, allocator, info);
        return texture;
    }
}
