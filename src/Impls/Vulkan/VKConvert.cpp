#include  "Impls/Vulkan/VKConvert.h"
#include "CacaoBuffer.h"
namespace Cacao
{
    vk::BufferUsageFlags Converter::Convert(BufferUsageFlags usage)
    {
        vk::BufferUsageFlags usageFlags;
        if (usage & BufferUsageFlags::VertexBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eVertexBuffer;
        if (usage & BufferUsageFlags::IndexBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eIndexBuffer;
        if (usage & BufferUsageFlags::UniformBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
        if (usage & BufferUsageFlags::StorageBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
        if (usage & BufferUsageFlags::TransferSrc)
            usageFlags |= vk::BufferUsageFlagBits::eTransferSrc;
        if (usage & BufferUsageFlags::TransferDst)
            usageFlags |= vk::BufferUsageFlagBits::eTransferDst;
        if (usage & BufferUsageFlags::IndirectBuffer)
            usageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
        if (usage & BufferUsageFlags::ShaderDeviceAddress)
            usageFlags |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
        if (usage & BufferUsageFlags::AccelerationStructure)
            usageFlags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        return usageFlags;
    }
    VmaMemoryUsage Converter::Convert(BufferMemoryUsage usage)
    {
        switch (usage)
        {
        case BufferMemoryUsage::GpuOnly:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferMemoryUsage::CpuOnly:
            return VMA_MEMORY_USAGE_CPU_ONLY;
        case BufferMemoryUsage::CpuToGpu:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case BufferMemoryUsage::GpuToCpu:
            return VMA_MEMORY_USAGE_GPU_TO_CPU;
        default:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        }
    }
    vk::Format Converter::Convert(Format format)
    {
        vk::Format res;
        switch (format)
        {
        case Format::RGBA8_UNORM: res = vk::Format::eR8G8B8A8Unorm;
            break;
        case Format::BGRA8_UNORM: res = vk::Format::eB8G8R8A8Unorm;
            break;
        case Format::RGBA8_SRGB: res = vk::Format::eR8G8B8A8Srgb;
            break;
        case Format::BGRA8_SRGB: res = vk::Format::eB8G8R8A8Srgb;
            break;
        case Format::RGBA16_FLOAT: res = vk::Format::eR16G16B16A16Sfloat;
            break;
        case Format::RGB10A2_UNORM: res = vk::Format::eA2B10G10R10UnormPack32;
            break;
        case Format::RGBA32_FLOAT: res = vk::Format::eR32G32B32A32Sfloat;
            break;
        case Format::R8_UNORM: res = vk::Format::eR8Unorm;
            break;
        case Format::R16_FLOAT: res = vk::Format::eR16Sfloat;
            break;
        case Format::D32F: res = vk::Format::eD32Sfloat;
            break;
        case Format::D24S8: res = vk::Format::eD24UnormS8Uint;
            break;
        default:
            throw std::runtime_error("Unsupported texture format in VKConvert");
        }
        return res;
    }
}
