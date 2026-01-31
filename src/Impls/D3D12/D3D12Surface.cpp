#include "../../../include/Impls/D3D12/D3D12Surface.h"

#include "Device.h"
#include "Impls/D3D12/D3D12Device.h"
#ifdef WIN32

namespace Cacao
{
    Ref<D3D12Surface> D3D12Surface::Create(const Ref<Instance>& inst, const NativeWindowHandle& windowHandle)
    {
        return CreateRef<D3D12Surface>(inst, windowHandle);
    }

    D3D12Surface::D3D12Surface(const Ref<Instance>& inst, const NativeWindowHandle& windowHandle)
    {
        m_hwnd = static_cast<HWND>(windowHandle.hWnd);
    }

    // ---------------------------------------------------------
    //  1. Surface Capabilities
    // ---------------------------------------------------------
    SurfaceCapabilities D3D12Surface::GetCapabilities(const Ref<Adapter>&)
    {
        SurfaceCapabilities caps{};

        caps.minImageCount = 2; // 所有系统保证支持的
        caps.maxImageCount = 3; // DXGI 一般 swapchain 3 张，安全默认
        caps.currentExtent = {0, 0}; // 由 swapchain 创建时决定
        caps.minImageExtent = {1, 1}; // 最小窗口
        caps.maxImageExtent = {16384, 16384}; // Windows 任意窗口都不会超过这个

        caps.currentTransform = {}; // Identity + 不翻转
        return caps;
    }

    // ---------------------------------------------------------
    //  2. 支持的颜色格式（返回最通用且所有系统都支持的）
    // ---------------------------------------------------------
    std::vector<SurfaceFormat> D3D12Surface::GetSupportedFormats(const Ref<Adapter>&)
    {
        return {
            {Format::RGBA8_UNORM, ColorSpace::SRGB_NONLINEAR},
            {Format::BGRA8_UNORM, ColorSpace::SRGB_NONLINEAR}
        };
    }

    // ---------------------------------------------------------
    //  3. 获取 Present Queue（默认 graphics queue）
    // ---------------------------------------------------------
    Ref<Queue> D3D12Surface::GetPresentQueue(const Ref<Device>& device)
    {
        // 默认 D3D12 由 graphics queue 执行 Present
        return std::static_pointer_cast<D3D12Device>(device)->GetQueue(QueueType::Present, 0);
    }

    // ---------------------------------------------------------
    //  4. 支持的 Present Mode（使用 DXGI 通用兼容值）
    // ---------------------------------------------------------
    std::vector<PresentMode> D3D12Surface::GetSupportedPresentModes(const Ref<Adapter>&)
    {
        // DXGI 对应 Vulkan：
        //  Immediate     = DXGI_SWAP_EFFECT_FLIP_DISCARD + tearing
        //  Fifo          = VSync
        //  Mailbox       = DXGI 不支持
        return {
            PresentMode::Immediate,
            PresentMode::Fifo,
        };
    }
}
#endif
