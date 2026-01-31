#ifdef WIN32
#include "Impls/D3D12/D3D12Texture.h"
#include "Impls/D3D12/D3D12Device.h"
#include <stdexcept>

namespace Cacao
{
    // 辅助函数
    static bool IsDepthFormat(Format format)
    {
        return format == Format::D16_UNORM || 
               format == Format::D32_FLOAT ||
               format == Format::D24_UNORM_S8_UINT ||
               format == Format::D32_FLOAT_S8_UINT;
    }

    static bool IsStencilFormat(Format format)
    {
        return format == Format::D24_UNORM_S8_UINT ||
               format == Format::D32_FLOAT_S8_UINT ||
               format == Format::S8_UINT;
    }

    // ========================= D3D12TextureView =========================

    D3D12TextureView::D3D12TextureView(const Ref<Texture>& texture, const TextureViewDesc& desc)
        : m_desc(desc)
    {
        if (!texture)
        {
            throw std::runtime_error("D3D12TextureView created with null texture");
        }
        m_texture = std::static_pointer_cast<D3D12Texture>(texture);
        
        // 从设备获取资源信息
        auto* d3d12Texture = static_cast<D3D12Texture*>(texture.get());
        ID3D12Resource* resource = d3d12Texture->GetResource();
        if (!resource) return;
        
        // 注意: 实际的描述符创建需要设备和描述符堆
        // 对于临时视图，我们记录视图信息，实际描述符在绑定时创建
    }

    D3D12TextureView::D3D12TextureView(const Ref<Texture>& texture, const TextureViewDesc& desc,
                                       D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
        : m_desc(desc), m_rtvHandle(rtvHandle), m_hasRTV(true)
    {
        if (!texture)
        {
            throw std::runtime_error("D3D12TextureView created with null texture");
        }
        m_texture = std::static_pointer_cast<D3D12Texture>(texture);
    }

    Ref<D3D12TextureView> D3D12TextureView::Create(const Ref<Texture>& texture, const TextureViewDesc& desc)
    {
        return CreateRef<D3D12TextureView>(texture, desc);
    }

    Ref<Texture> D3D12TextureView::GetTexture() const
    {
        return m_texture;
    }

    const TextureViewDesc& D3D12TextureView::GetDesc() const
    {
        return m_desc;
    }

    // ========================= D3D12Texture =========================

    static D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType type)
    {
        switch (type)
        {
        case TextureType::Texture1D:
        case TextureType::Texture1DArray:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        case TextureType::Texture2D:
        case TextureType::Texture2DArray:
        case TextureType::TextureCube:
        case TextureType::TextureCubeArray:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case TextureType::Texture3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        default:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        }
    }

    static D3D12_RESOURCE_FLAGS GetResourceFlags(TextureUsageFlags usage)
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        
        if (usage & TextureUsageFlags::ColorAttachment)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (usage & TextureUsageFlags::DepthStencilAttachment)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (usage & TextureUsageFlags::Storage)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        
        // 如果不用于采样，则标记为拒绝着色器资源
        if (!(usage & TextureUsageFlags::Sampled) && 
            (usage & TextureUsageFlags::DepthStencilAttachment))
        {
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        
        return flags;
    }

    D3D12Texture::D3D12Texture(const ComPtr<ID3D12Resource>& resource, const TextureCreateInfo& info)
        : m_resource(resource), m_createInfo(info)
    {
        // 从交换链创建的纹理，不拥有资源
        m_allocation = nullptr;
        m_currentState = D3D12_RESOURCE_STATE_PRESENT;
    }

    Ref<D3D12Texture> D3D12Texture::CreateFromSwapchainImage(const ComPtr<ID3D12Resource>& resource,
                                                              const TextureCreateInfo& info)
    {
        auto texture = CreateRef<D3D12Texture>(resource, info);
        return texture;
    }

    D3D12Texture::D3D12Texture(const Ref<Device>& device, D3D12MA::Allocator* allocator, 
                               const TextureCreateInfo& info)
        : m_createInfo(info)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12Texture created with null device");
        }
        
        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12Texture requires a D3D12Device");
        }
        
        if (!allocator)
        {
            throw std::runtime_error("D3D12Texture requires a valid allocator");
        }

        // 创建资源描述
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = GetResourceDimension(info.Type);
        resourceDesc.Alignment = 0;
        resourceDesc.Width = info.Width;
        resourceDesc.Height = info.Height;
        resourceDesc.DepthOrArraySize = (info.Type == TextureType::Texture3D) 
            ? static_cast<UINT16>(info.Depth)
            : static_cast<UINT16>(info.ArrayLayers);
        resourceDesc.MipLevels = static_cast<UINT16>(info.MipLevels);
        resourceDesc.Format = D3D12Converter::Convert(info.Format);
        resourceDesc.SampleDesc.Count = static_cast<UINT>(info.SampleCount);
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = GetResourceFlags(info.Usage);

        // 确定初始状态
        D3D12_RESOURCE_STATES initialState = D3D12Converter::Convert(info.InitialLayout);
        m_currentState = initialState;

        // 创建分配信息
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        // 设置清除值（用于渲染目标和深度缓冲）
        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;
        
        if (info.Usage & TextureUsageFlags::ColorAttachment)
        {
            clearValue.Format = resourceDesc.Format;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;
            pClearValue = &clearValue;
        }
        else if (info.Usage & TextureUsageFlags::DepthStencilAttachment)
        {
            clearValue.Format = resourceDesc.Format;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;
            pClearValue = &clearValue;
        }

        // 使用 D3D12MA 创建资源
        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            initialState,
            pClearValue,
            &m_allocation,
            IID_PPV_ARGS(&m_resource));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 texture resource");
        }

        // 设置资源名称（用于调试）
        if (!info.Name.empty())
        {
            std::wstring wname(info.Name.begin(), info.Name.end());
            m_resource->SetName(wname.c_str());
        }
    }

    Ref<D3D12Texture> D3D12Texture::Create(const Ref<Device>& device, D3D12MA::Allocator* allocator,
                                           const TextureCreateInfo& info)
    {
        auto texture = CreateRef<D3D12Texture>(device, allocator, info);
        texture->CreateDefaultViewIfNeeded();
        return texture;
    }

    D3D12Texture::~D3D12Texture()
    {
        m_defaultView.reset();
        
        if (m_allocation)
        {
            m_allocation->Release();
            m_allocation = nullptr;
        }
        
        m_resource.Reset();
    }

    void D3D12Texture::CreateDefaultViewIfNeeded()
    {
        if (m_defaultView)
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
        
        // 确定纵横比掩码
        AspectMask aspectFlags = {};
        if (IsDepthFormat(m_createInfo.Format)) aspectFlags |= AspectMask::Depth;
        if (IsStencilFormat(m_createInfo.Format)) aspectFlags |= AspectMask::Stencil;
        if (!aspectFlags) aspectFlags = AspectMask::Color;
        viewDesc.Aspect = aspectFlags;
        
        m_defaultView = D3D12TextureView::Create(shared_from_this(), viewDesc);
    }

    uint32_t D3D12Texture::GetWidth() const { return m_createInfo.Width; }
    uint32_t D3D12Texture::GetHeight() const { return m_createInfo.Height; }
    uint32_t D3D12Texture::GetDepth() const { return m_createInfo.Depth; }
    uint32_t D3D12Texture::GetMipLevels() const { return m_createInfo.MipLevels; }
    uint32_t D3D12Texture::GetArrayLayers() const { return m_createInfo.ArrayLayers; }
    Format D3D12Texture::GetFormat() const { return m_createInfo.Format; }
    TextureType D3D12Texture::GetType() const { return m_createInfo.Type; }
    SampleCount D3D12Texture::GetSampleCount() const { return m_createInfo.SampleCount; }
    TextureUsageFlags D3D12Texture::GetUsage() const { return m_createInfo.Usage; }
    
    ImageLayout D3D12Texture::GetCurrentLayout() const 
    { 
        // 根据当前 D3D12 状态返回对应的布局
        switch (m_currentState)
        {
        case D3D12_RESOURCE_STATE_RENDER_TARGET:
            return ImageLayout::ColorAttachment;
        case D3D12_RESOURCE_STATE_DEPTH_WRITE:
            return ImageLayout::DepthStencilAttachment;
        case D3D12_RESOURCE_STATE_DEPTH_READ:
            return ImageLayout::DepthStencilReadOnly;
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
            return ImageLayout::ShaderReadOnly;
        case D3D12_RESOURCE_STATE_COPY_SOURCE:
            return ImageLayout::TransferSrc;
        case D3D12_RESOURCE_STATE_COPY_DEST:
            return ImageLayout::TransferDst;
        case D3D12_RESOURCE_STATE_PRESENT:
            return ImageLayout::Present;
        default:
            return ImageLayout::General;
        }
    }

    Ref<CacaoTextureView> D3D12Texture::CreateView(const TextureViewDesc& desc)
    {
        return D3D12TextureView::Create(shared_from_this(), desc);
    }

    Ref<CacaoTextureView> D3D12Texture::GetDefaultView()
    {
        return m_defaultView;
    }
}
#endif
