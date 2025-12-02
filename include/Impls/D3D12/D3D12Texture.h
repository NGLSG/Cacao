#ifndef CACAO_D3D12TEXTURE_H
#define CACAO_D3D12TEXTURE_H
#ifdef WIN32
#include "Texture.h"
#include  "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Texture : public Texture
    {
        ComPtr<ID3D12Resource> m_d3d12Resource;
    public:
        Format GetFormat() const override;
        TextureUsageFlags GetUsage() const override;
        TextureType GetType() const override;
        uint32_t GetMipLevels() const override;
        uint32_t GetArrayLayers() const override;
        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        uint32_t GetDepth() const override;
        SampleCount GetSampleCount() const override;
        ImageLayout GetCurrentLayout() const override;
        Ref<CacaoTextureView> CreateView(const TextureViewDesc& desc) override;
        Ref<CacaoTextureView> GetDefaultView() override;
        void CreateDefaultViewIfNeeded() override;
    };
} 
#endif
#endif 
