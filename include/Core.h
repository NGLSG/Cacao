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
} 
#endif 
