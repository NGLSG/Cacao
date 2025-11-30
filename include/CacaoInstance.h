#ifndef CACAO_CACAOINSTANCE_H
#define CACAO_CACAOINSTANCE_H
namespace Cacao
{
    class CacaoSurface;
}
namespace Cacao
{
    class CacaoAdapter;
    struct NativeWindowHandle
    {
#if defined(_WIN32)
        void* hWnd = nullptr; 
        void* hInst = nullptr; 
#elif defined(__APPLE__)
        void* metalLayer = nullptr; 
#elif defined(__linux__) && !defined(__ANDROID__)
#if defined(USE_WAYLAND)
        wl_display* wlDisplay = nullptr; 
        wl_surface* wlSurface = nullptr; 
#elif defined(USE_XCB)
        xcb_connection_t* xcbConnection = nullptr; 
        xcb_window_t xcbWindow = 0; 
#elif defined(USE_XLIB)
        Display* x11Display = nullptr; 
        Window x11Window = 0; 
#endif
#elif defined(__ANDROID__)
        void* aNativeWindow = nullptr; 
#else
        void* placeholder = nullptr; 
#endif
        bool IsValid() const
        {
#if defined(_WIN32)
            return hWnd != nullptr && hInst != nullptr;
#elif defined(__APPLE__)
            return metalLayer != nullptr;
#elif defined(__linux__) && !defined(__ANDROID__)
#if defined(USE_WAYLAND)
            return wlDisplay != nullptr && wlSurface != nullptr;
#elif defined(USE_XCB)
            return xcbConnection != nullptr && xcbWindow != 0;
#elif defined(USE_XLIB)
            return x11Display != nullptr && x11Window != 0;
#endif
#elif defined(__ANDROID__)
            return aNativeWindow != nullptr;
#else
            return placeholder != nullptr;
#endif
        }
    };
    enum class CacaoType
    {
        Vulkan,
        DirectX12,
        DirectX11,
        Metal,
        OpenGL,
        OpenGLES,
    };
    enum class CacaoInstanceFeature
    {
        ValidationLayer = 0x00000001,
        Surface = 0x00000002,
        RayTracing = 0x00000004,
        MeshShader = 0x00000008,
        BindlessDescriptors = 0x00000016,
    };
    struct CacaoInstanceCreateInfo
    {
        CacaoType type = CacaoType::Vulkan;
        std::string applicationName;
        uint32_t appVersion = 1;
        std::vector<CacaoInstanceFeature> enabledFeatures;
    };
    class CACAO_API CacaoInstance: public std::enable_shared_from_this<CacaoInstance>
    {
    public:
        virtual ~CacaoInstance() = default;
        static Ref<CacaoInstance> Create(const CacaoInstanceCreateInfo& createInfo);
        [[nodiscard]] virtual CacaoType GetType() const = 0;
        virtual bool Initialize(const CacaoInstanceCreateInfo& createInfo) = 0;
        virtual std::vector<Ref<CacaoAdapter>> EnumerateAdapters() = 0;
        virtual bool IsFeatureEnabled(CacaoInstanceFeature feature) const = 0;
        virtual Ref<CacaoSurface> CreateSurface(const NativeWindowHandle& windowHandle) = 0;
    };
    template <>
    struct to_string<CacaoType>
    {
        static std::string convert(CacaoType type)
        {
            switch (type)
            {
            case CacaoType::Vulkan:
                return "Vulkan";
            case CacaoType::DirectX12:
                return "DirectX12";
            case CacaoType::DirectX11:
                return "DirectX11";
            case CacaoType::Metal:
                return "Metal";
            case CacaoType::OpenGL:
                return "OpenGL";
            case CacaoType::OpenGLES:
                return "OpenGLES";
            default:
                return "Unknown";
            }
        }
    };
    template <>
    struct to_string<CacaoInstanceFeature>
    {
        static std::string convert(CacaoInstanceFeature feature)
        {
            switch (feature)
            {
            case CacaoInstanceFeature::ValidationLayer:
                return "ValidationLayer";
            case CacaoInstanceFeature::Surface:
                return "Surface";
            case CacaoInstanceFeature::RayTracing:
                return "RayTracing";
            case CacaoInstanceFeature::MeshShader:
                return "MeshShader";
            case CacaoInstanceFeature::BindlessDescriptors:
                return "BindlessDescriptors";
            default:
                return "Unknown";
            }
        }
    };
}
#endif 
