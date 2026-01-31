#ifndef CACAO_D3D12TEXTURE_H
#define CACAO_D3D12TEXTURE_H
#ifdef WIN32
#include "D3D12MemAlloc.h"
#include "Texture.h"
#include "D3D12Common.h"

namespace Cacao
{
    class D3D12Device;
    class D3D12Texture;

    class CACAO_API D3D12TextureView : public CacaoTextureView
    {
        Ref<D3D12Texture> m_texture;
        TextureViewDesc m_desc;
        
        // D3D12 描述符句柄 - 在描述符堆中的位置
        D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle = {};
        
        bool m_hasSRV = false;
        bool m_hasUAV = false;
        bool m_hasRTV = false;
        bool m_hasDSV = false;

        friend class D3D12CommandBufferEncoder;
        friend class D3D12DescriptorSet;
        friend class D3D12Swapchain;

    public:
        D3D12TextureView(const Ref<Texture>& texture, const TextureViewDesc& desc);
        static Ref<D3D12TextureView> Create(const Ref<Texture>& texture, const TextureViewDesc& desc);
        
        // 从交换链创建的特殊构造函数
        D3D12TextureView(const Ref<Texture>& texture, const TextureViewDesc& desc, 
                         D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
        
        Ref<Texture> GetTexture() const override;
        const TextureViewDesc& GetDesc() const override;
        
        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_srvHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetUAVHandle() const { return m_uavHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_rtvHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_dsvHandle; }
        
        bool HasSRV() const { return m_hasSRV; }
        bool HasUAV() const { return m_hasUAV; }
        bool HasRTV() const { return m_hasRTV; }
        bool HasDSV() const { return m_hasDSV; }
    };

    class CACAO_API D3D12Texture final : public Texture
    {
        friend class D3D12Swapchain;
        friend class D3D12TextureView;
        friend class D3D12CommandBufferEncoder;
        friend class D3D12DescriptorSet;

        ComPtr<ID3D12Resource> m_resource;
        D3D12MA::Allocation* m_allocation = nullptr;
        Ref<D3D12Device> m_device;
        TextureCreateInfo m_createInfo;
        Ref<D3D12TextureView> m_defaultView;
        
        // 跟踪当前资源状态
        D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;

    public:
        // 从交换链图像创建（不拥有资源）
        D3D12Texture(const ComPtr<ID3D12Resource>& resource, const TextureCreateInfo& info);
        static Ref<D3D12Texture> CreateFromSwapchainImage(const ComPtr<ID3D12Resource>& resource, 
                                                           const TextureCreateInfo& info);
        
        // 正常创建（拥有资源）
        D3D12Texture(const Ref<Device>& device, D3D12MA::Allocator* allocator, const TextureCreateInfo& info);
        static Ref<D3D12Texture> Create(const Ref<Device>& device, D3D12MA::Allocator* allocator, 
                                        const TextureCreateInfo& info);
        
        ~D3D12Texture() override;

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
        Ref<CacaoTextureView> GetDefaultView() override;
        void CreateDefaultViewIfNeeded() override;
        
        ID3D12Resource* GetResource() const { return m_resource.Get(); }
        D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
        void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }
    };
}
#endif
#endif 
