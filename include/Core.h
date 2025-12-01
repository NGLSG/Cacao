#ifndef CACAO_CORE_H
#define CACAO_CORE_H
#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>
#ifdef _WIN32
#ifdef CACAO_BUILD_DLL
#define CACAO_API __declspec(dllexport)
#else
#define CACAO_API __declspec(dllimport)
#endif
#else
#ifdef CACAO_BUILD_DLL
#define CACAO_API __attribute__((visibility("default")))
#else
#define CACAO_API
#endif
#endif
namespace Cacao
{
    template <typename T>
    using Box = std::unique_ptr<T>;
    template <typename T>
    using Ref = std::shared_ptr<T>;
    template <typename T, typename... Args>
    constexpr Box<T> CreateBox(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    template <typename T>
    struct to_string
    {
        static std::string Convert(const T& value)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                return std::to_string(value);
            }
            else
            {
                return "[Unknown Type]";
            }
        }
    };
    template <typename T>
    struct StringProxy
    {
        const T& Value;
    };
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const StringProxy<T>& proxy)
    {
        os << to_string<T>::Format(proxy.Value);
        return os;
    }
    template <typename T>
    StringProxy<T> ToString(const T& value)
    {
        return StringProxy<T>{value};
    }
    enum class Format
    {
        RGBA8_SRGB,
        RGBA8_UNORM,
        BGRA8_SRGB,
        BGRA8_UNORM,
        RGBA16_FLOAT,
        RGB10A2_UNORM,
        RGB16_FLOAT,
        RGBA32_FLOAT,
        R8_UNORM,
        R16_FLOAT,
        D24S8,
        D32F,
        UNDEFINED,
    };
    enum class ColorSpace
    {
        SRGB_NONLINEAR,
        HDR10_ST2084,
        LINEAR,
        BT2020_NONLINEAR,
        DISPLAY_P3,
    };
    struct Extent2D
    {
        uint32_t width;
        uint32_t height;
    };
    enum class ShaderStage : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2,
        Geometry = 1 << 3,
        TessellationControl = 1 << 4,
        TessellationEvaluation = 1 << 5,
        RayGen = 1 << 6,
        RayAnyHit = 1 << 7,
        RayClosestHit = 1 << 8,
        RayMiss = 1 << 9,
        RayIntersection = 1 << 10,
        Callable = 1 << 11,
        Mesh = 1 << 12,
        Task = 1 << 13,
        AllGraphics = Vertex | Fragment | Geometry | TessellationControl | TessellationEvaluation,
        AllRayTracing = RayGen | RayAnyHit | RayClosestHit | RayMiss | RayIntersection | Callable,
        AllMeshShading = Mesh | Task,
        All = 0xFFFFFFFF
    };
    inline ShaderStage operator|(ShaderStage a, ShaderStage b)
    {
        return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    inline ShaderStage& operator|=(ShaderStage& a, ShaderStage b)
    {
        a = a | b;
        return a;
    }
    inline bool operator&(ShaderStage a, ShaderStage b)
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }
}
#endif
