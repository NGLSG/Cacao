#ifndef CACAO_VKCONVERT_H
#define CACAO_VKCONVERT_H
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
namespace Cacao
{
    enum class BufferMemoryUsage;
    enum class BufferUsageFlags : uint32_t;
    class Converter
    {
    public:
        static vk::BufferUsageFlags Convert(BufferUsageFlags usage);
        static VmaMemoryUsage Convert(BufferMemoryUsage usage);
        static vk::Format Convert(Format format);
    };
}
#endif 
