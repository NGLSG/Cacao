#ifndef CACAO_CACAOTEXTURE_H
#define CACAO_CACAOTEXTURE_H
#include "CacaoBarrier.h"
namespace Cacao
{
    enum class TextureType
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube, 
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray
    };
    enum class TextureUsageFlags : uint32_t
    {
        None = 0,
        TransferSrc = 1 << 0, 
        TransferDst = 1 << 1, 
        Sampled = 1 << 2, 
        Storage = 1 << 3, 
        ColorAttachment = 1 << 4, 
        DepthStencilAttachment = 1 << 5, 
        TransientAttachment = 1 << 6, 
        InputAttachment = 1 << 7 
    };
    inline TextureUsageFlags operator|(TextureUsageFlags a, TextureUsageFlags b)
    {
        return static_cast<TextureUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline TextureUsageFlags& operator|=(TextureUsageFlags& a, TextureUsageFlags b)
    {
        a = a | b;
        return a;
    }
    inline bool operator &(TextureUsageFlags a, TextureUsageFlags b)
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }
    enum class SampleCount : uint32_t
    {
        Count1 = 1,
        Count2 = 2,
        Count4 = 4,
        Count8 = 8,
        Count16 = 16,
        Count32 = 32,
        Count64 = 64
    };
    struct TextureCreateInfo
    {
        TextureType Type = TextureType::Texture2D;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t Depth = 1; 
        uint32_t ArrayLayers = 1; 
        uint32_t MipLevels = 1; 
        Format Format = Format::RGBA8_UNORM;
        TextureUsageFlags Usage = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst;
        ImageLayout InitialLayout = ImageLayout::Undefined;
        SampleCount SampleCount = SampleCount::Count1;
        std::string Name;
        void* InitialData = nullptr;
    };
    class CACAO_API CacaoTexture
    {
    public:
        virtual ~CacaoTexture() = default;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetDepth() const = 0;
        virtual uint32_t GetMipLevels() const = 0;
        virtual uint32_t GetArrayLayers() const = 0;
        virtual Format GetFormat() const = 0;
        virtual TextureType GetType() const = 0;
        virtual SampleCount GetSampleCount() const = 0;
        virtual TextureUsageFlags GetUsage() const = 0;
        virtual ImageLayout GetCurrentLayout() const = 0;
        bool IsDepthStencil() const
        {
            return IsDepthFormat(GetFormat()) || IsStencilFormat(GetFormat());
        }
        bool HasDepth() const { return IsDepthFormat(GetFormat()); }
        bool HasStencil() const { return IsStencilFormat(GetFormat()); }
    protected:
        static bool IsDepthFormat(Format format)
        {
            return format == Format::D32F;
        }
        static bool IsStencilFormat(Format format)
        {
            return format == Format::D24S8;
        }
    };
}
#endif 
