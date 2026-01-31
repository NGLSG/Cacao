#ifdef WIN32
#include "Impls/D3D12/D3D12Swapchain.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Surface.h"
#include "Impls/D3D12/D3D12Texture.h"
#include "Impls/D3D12/D3D12Queue.h"
#include "Impls/D3D12/D3D12Synchronization.h"
#include "Impls/D3D12/D3D12Adapter.h"
#include "Impls/D3D12/D3D12Instance.h"
#include <stdexcept>

namespace Cacao
{
    static DXGI_SWAP_EFFECT GetSwapEffect(PresentMode mode, bool tearingSupported)
    {
        // FLIP_DISCARD 是现代 Windows 推荐的模式
        return DXGI_SWAP_EFFECT_FLIP_DISCARD;
    }

    static UINT GetSyncInterval(PresentMode mode)
    {
        switch (mode)
        {
        case PresentMode::Immediate:
            return 0;
        case PresentMode::Mailbox:
            return 0; // Mailbox 在 DXGI 中通过 ALLOW_TEARING 实现
        case PresentMode::Fifo:
        case PresentMode::FifoRelaxed:
        default:
            return 1;
        }
    }

    D3D12Swapchain::D3D12Swapchain(const Ref<Device>& device, const SwapchainCreateInfo& createInfo)
        : m_createInfo(createInfo)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12Swapchain created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12Swapchain requires a D3D12Device");
        }

        if (!createInfo.CompatibleSurface)
        {
            throw std::runtime_error("D3D12Swapchain requires a valid surface");
        }

        auto* d3d12Surface = dynamic_cast<D3D12Surface*>(createInfo.CompatibleSurface.get());
        if (!d3d12Surface)
        {
            throw std::runtime_error("D3D12Swapchain requires a D3D12Surface");
        }

        HWND hwnd = d3d12Surface->GetHWND();
        if (!hwnd)
        {
            throw std::runtime_error("D3D12Swapchain: Surface has invalid HWND");
        }

        // 获取 DXGI Factory
        auto adapter = std::dynamic_pointer_cast<D3D12Adapter>(m_device->GetParentAdapter());
        auto instance = std::dynamic_pointer_cast<D3D12Instance>(adapter->GetParentInstance());
        IDXGIFactory7* factory = instance->GetDXGIFactory().Get();

        // 检查 tearing 支持
        BOOL tearingSupport = FALSE;
        if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
            &tearingSupport, sizeof(tearingSupport))))
        {
            m_tearingSupported = (tearingSupport == TRUE);
        }

        // 创建交换链描述
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = createInfo.Extent.width;
        swapchainDesc.Height = createInfo.Extent.height;
        swapchainDesc.Format = D3D12Converter::Convert(createInfo.Format);
        swapchainDesc.Stereo = FALSE;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = createInfo.MinImageCount;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.SwapEffect = GetSwapEffect(createInfo.PresentMode, m_tearingSupported);
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchainDesc.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        // 获取命令队列
        auto graphicsQueue = std::dynamic_pointer_cast<D3D12Queue>(m_device->GetQueue(QueueType::Graphics, 0));
        if (!graphicsQueue)
        {
            throw std::runtime_error("D3D12Swapchain: Failed to get graphics queue");
        }

        // 创建交换链
        ComPtr<IDXGISwapChain1> swapchain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            graphicsQueue->GetD3D12CommandQueue(),
            hwnd,
            &swapchainDesc,
            nullptr,
            nullptr,
            &swapchain1);

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 swapchain");
        }

        // 禁用 Alt+Enter 全屏切换
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        // 获取 IDXGISwapChain4 接口
        hr = swapchain1.As(&m_swapchain);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get IDXGISwapChain4 interface");
        }

        // 创建 RTV 描述符堆
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = createInfo.MinImageCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = m_device->GetHandle()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create RTV descriptor heap");
        }

        // 获取 RTV 描述符大小
        UINT rtvDescriptorSize = m_device->GetHandle()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // 创建后备缓冲区和 RTV
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        m_backBuffers.resize(createInfo.MinImageCount);
        m_rtvHandles.resize(createInfo.MinImageCount);

        for (UINT i = 0; i < createInfo.MinImageCount; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to get swapchain buffer");
            }

            // 创建 RTV
            m_device->GetHandle()->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
            m_rtvHandles[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;

            // 创建纹理包装器
            TextureCreateInfo texInfo = {};
            texInfo.Type = TextureType::Texture2D;
            texInfo.Width = createInfo.Extent.width;
            texInfo.Height = createInfo.Extent.height;
            texInfo.Depth = 1;
            texInfo.ArrayLayers = 1;
            texInfo.MipLevels = 1;
            texInfo.Format = createInfo.Format;
            texInfo.Usage = TextureUsageFlags::ColorAttachment;
            texInfo.InitialLayout = ImageLayout::Present;
            texInfo.SampleCount = SampleCount::Count1;
            texInfo.Name = "SwapchainBackBuffer";

            auto texture = D3D12Texture::CreateFromSwapchainImage(backBuffer, texInfo);

            // 为后备缓冲区创建带有RTV句柄的默认视图
            TextureViewDesc viewDesc = {};
            viewDesc.BaseMipLevel = 0;
            viewDesc.MipLevelCount = 1;
            viewDesc.BaseArrayLayer = 0;
            viewDesc.ArrayLayerCount = 1;
            viewDesc.Aspect = AspectMask::Color;

            texture->m_defaultView = CreateRef<D3D12TextureView>(texture, viewDesc, m_rtvHandles[i]);

            m_backBuffers[i] = texture;
        }

        m_currentBackBufferIndex = m_swapchain->GetCurrentBackBufferIndex();
    }

    D3D12Swapchain::~D3D12Swapchain()
    {
        m_backBuffers.clear();
        m_rtvHandles.clear();
        m_rtvHeap.Reset();
        m_swapchain.Reset();
    }

    Result D3D12Swapchain::Present(const Ref<Queue>& queue, const Ref<Synchronization>& sync, uint32_t frameIndex)
    {
        UINT syncInterval = GetSyncInterval(m_createInfo.PresentMode);
        UINT presentFlags = 0;

        if (m_tearingSupported && syncInterval == 0)
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        HRESULT hr = m_swapchain->Present(syncInterval, presentFlags);

        // 更新当前后备缓冲区索引
        m_currentBackBufferIndex = m_swapchain->GetCurrentBackBufferIndex();

        if (SUCCEEDED(hr))
        {
            return Result::Success;
        }
        else if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            return Result::DeviceLost;
        }
        else if (hr == DXGI_STATUS_OCCLUDED)
        {
            return Result::Suboptimal;
        }
        else
        {
            return Result::Error;
        }
    }

    uint32_t D3D12Swapchain::GetImageCount() const
    {
        return static_cast<uint32_t>(m_backBuffers.size());
    }

    Ref<Texture> D3D12Swapchain::GetBackBuffer(uint32_t index) const
    {
        if (index >= m_backBuffers.size())
        {
            throw std::runtime_error("Invalid back buffer index");
        }
        return m_backBuffers[index];
    }

    Extent2D D3D12Swapchain::GetExtent() const
    {
        return m_createInfo.Extent;
    }

    Format D3D12Swapchain::GetFormat() const
    {
        return m_createInfo.Format;
    }

    PresentMode D3D12Swapchain::GetPresentMode() const
    {
        return m_createInfo.PresentMode;
    }

    Result D3D12Swapchain::AcquireNextImage(const Ref<Synchronization>& sync, int idx, int& out)
    {
        // D3D12 中，当前后备缓冲区索引由交换链自动管理
        out = static_cast<int>(m_currentBackBufferIndex);
        return Result::Success;
    }

    Ref<Swapchain> D3D12Swapchain::Create(const Ref<Device>& device, const SwapchainCreateInfo& createInfo)
    {
        return CreateRef<D3D12Swapchain>(device, createInfo);
    }
}
#endif
