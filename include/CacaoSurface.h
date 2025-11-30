#ifndef CACAO_CACAOSURFACE_H
#define CACAO_CACAOSURFACE_H
namespace Cacao
{
    class CacaoDevice;
    class CacaoQueue;
    class CacaoAdapter;
}
namespace Cacao
{
    enum class SurfaceRotation
    {
        Identity, 
        Rotate90, 
        Rotate180, 
        Rotate270 
    };
    struct SurfaceTransform
    {
        SurfaceRotation rotation = SurfaceRotation::Identity;
        bool flipHorizontal = false;
        bool flipVertical = false;
        bool IsSwappedDimensions() const
        {
            return rotation == SurfaceRotation::Rotate90 || rotation == SurfaceRotation::Rotate270;
        }
    };
    struct SurfaceFormat
    {
        Format format;
        ColorSpace colorSpace;
    };
    struct SurfaceCapabilities
    {
        uint32_t minImageCount; 
        uint32_t maxImageCount; 
        Extent2D currentExtent; 
        Extent2D minImageExtent; 
        Extent2D maxImageExtent; 
        SurfaceTransform currentTransform;
    };
    enum class PresentMode
    {
        Immediate, 
        Mailbox, 
        Fifo, 
        FifoRelaxed 
    };
    class CACAO_API CacaoSurface : public std::enable_shared_from_this<CacaoSurface>
    {
    public :
        virtual ~CacaoSurface() = default;
        virtual SurfaceCapabilities GetCapabilities(const Ref<CacaoAdapter>& adapter) = 0;
        virtual std::vector<SurfaceFormat> GetSupportedFormats(const Ref<CacaoAdapter>& adapter) = 0;
        virtual Ref<CacaoQueue> GetPresentQueue(const Ref<CacaoDevice>& device) =0;
        virtual std::vector<PresentMode> GetSupportedPresentModes(const Ref<CacaoAdapter>& adapter) = 0;
    };
}
#endif 
