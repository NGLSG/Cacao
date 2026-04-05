// Cacao Compute Demo - Mandelbrot 分形
// 演示 Compute Pipeline、Storage Image、Dispatch、Barrier+Copy 工作流
// 鼠标左键拖动平移，滚轮缩放

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include "Cacao.hpp"
#ifdef CACAO_USE_VULKAN
#include "Impls/Vulkan/VKTexture.h"
#else
#include "Impls/D3D12/D3D12Texture.h"
#endif

using namespace Cacao;

// 交互状态（鼠标拖动平移 + 滚轮缩放）
static float g_centerX = -0.5f, g_centerY = 0.0f;
static float g_zoom = 1.0f;
static bool g_dragging = false;
static int g_lastMouseX = 0, g_lastMouseY = 0;

// 与 Slang shader 中的 Params 结构体布局一致（256 字节对齐的 UBO）
struct MandelbrotParams
{
    uint32_t width;
    uint32_t height;
    float centerX;
    float centerY;
    float zoom;
    uint32_t maxIter;
    float _pad0;
    float _pad1;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_LBUTTONDOWN) {
        g_dragging = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        SetCapture(hwnd);
        return 0;
    }
    if (uMsg == WM_LBUTTONUP) {
        g_dragging = false;
        ReleaseCapture();
        return 0;
    }
    if (uMsg == WM_MOUSEMOVE && g_dragging) {
        int mx = LOWORD(lParam), my = HIWORD(lParam);
        g_centerX -= (float)(mx - g_lastMouseX) * 0.003f / g_zoom;
        g_centerY -= (float)(my - g_lastMouseY) * 0.003f / g_zoom;
        g_lastMouseX = mx;
        g_lastMouseY = my;
        return 0;
    }
    if (uMsg == WM_MOUSEWHEEL) {
        float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
        g_zoom *= (delta > 0) ? 1.2f : (1.0f / 1.2f);
        if (g_zoom < 0.1f) g_zoom = 0.1f;
        if (g_zoom > 1e8f) g_zoom = 1e8f;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const char* className = "ComputeDemoClass";
    BackendType selectedBackend = BackendType::DirectX12;

    // === Win32 窗口创建（与 hello_triangle 相同模式）===
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = className;
    RegisterClassEx(&wc);

    RECT windowRect = {0, 0, 1024, 768};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(0, className, "Cacao - Mandelbrot Compute Demo",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    std::cout << "=== Cacao Mandelbrot Compute Demo ===" << std::endl;

    // === Cacao RHI 初始化 ===
    // 创建 Instance → 枚举 Adapter → 创建 Surface → 创建 Device
    InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = selectedBackend;
    instanceCreateInfo.applicationName = "ComputeDemo";
    instanceCreateInfo.appVersion = 1;
    instanceCreateInfo.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    Ref<Instance> instance = Instance::Create(instanceCreateInfo);

    Ref<Adapter> adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<Surface> surface = instance->CreateSurface(nativeWindowHandle);

    DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<Device> device = adapter->CreateDevice(deviceCreateInfo);

    // === 创建 Swapchain ===
    auto surfaceCaps = surface->GetCapabilities(adapter);
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (imageCount > surfaceCaps.maxImageCount && surfaceCaps.maxImageCount != 0)
        imageCount = surfaceCaps.maxImageCount;

    Ref<Swapchain> swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(surfaceCaps.currentExtent)
        .SetFormat(Format::RGBA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo)
        .SetMinImageCount(imageCount)
        .SetSurface(surface)
        .SetPreTransform(surfaceCaps.currentTransform)
        .SetClipped(true)
        .Build());

    // 帧同步：framesInFlight = swapchain 图像数 - 1
    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    Ref<Synchronization> synchronization = device->CreateSynchronization(framesInFlight);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    auto swapExtent = swapchain->GetExtent();
    const uint32_t rtW = swapExtent.width, rtH = swapExtent.height;
    std::cout << "Swapchain: " << rtW << "x" << rtH << std::endl;

    // === 创建 Storage Image（Compute Shader 的输出目标）===
    // 需要 Storage（UAV写入）+ TransferSrc（复制到 backbuffer）+ Sampled
    auto outputImage = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(rtW, rtH)
        .SetFormat(Format::RGBA8_UNORM)
        .SetUsage(TextureUsageFlags::Storage | TextureUsageFlags::TransferSrc | TextureUsageFlags::Sampled)
        .SetName("MandelbrotOutput")
        .Build());
    outputImage->CreateDefaultViewIfNeeded();

    // === 初始化 Storage Image 到 UAV 状态 ===
    // Compute Shader 写入前，纹理必须处于 UnorderedAccess 状态
    {
        auto initCmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
        initCmd->Begin({true});
#ifdef CACAO_USE_VULKAN
        // Vulkan: 使用跨平台 TextureBarrier API
        TextureBarrier initBarrier;
        initBarrier.Texture = outputImage;
        initBarrier.OldState = ResourceState::Undefined;
        initBarrier.NewState = ResourceState::UnorderedAccess;
        initCmd->PipelineBarrier(SyncScope::AllCommands, SyncScope::AllCommands, std::span<const TextureBarrier>(&initBarrier, 1));
#else
        // DX12: 使用原生 D3D12 Resource Barrier（COMMON → UNORDERED_ACCESS）
        initCmd->ExecuteNative([&](void* h) {
            auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(h);
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = static_cast<D3D12Texture*>(outputImage.get())->GetHandle();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            cmdList->ResourceBarrier(1, &barrier);
        });
#endif
        initCmd->End();
        graphicsQueue->Submit(initCmd);
        graphicsQueue->WaitIdle();
    }

    // === 创建 Uniform Buffer（传递分形参数到 shader）===
    // 大小对齐到 256 字节（D3D12 CBV 要求）
    const uint64_t cbSize = (sizeof(MandelbrotParams) + 255) & ~255;
    auto paramBuffer = device->CreateBuffer(BufferBuilder()
        .SetSize(cbSize)
        .SetUsage(BufferUsageFlags::UniformBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("MandelbrotParams")
        .Build());

    // === 编译 Compute Shader（Slang → DXIL/SPIRV）===
    // 清除旧缓存确保使用最新 shader
    {
        namespace fs = std::filesystem;
        if (fs::exists("shader_cache_compute"))
            fs::remove_all("shader_cache_compute");
    }

    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_compute");

    // 搜索 shader 文件路径（exe 目录 → demos 目录 → 绝对路径）
    std::string shaderPath = "compute_demo.slang";
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        auto sep = exeDir.find_last_of("\\/");
        if (sep != std::string::npos) exeDir = exeDir.substr(0, sep + 1);
        std::string candidates[] = {
            exeDir + "compute_demo.slang",
            exeDir + "..\\demos\\compute_demo.slang",
            exeDir + "..\\..\\demos\\compute_demo.slang",
            "demos\\compute_demo.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\compute_demo.slang"
        };
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) { shaderPath = c; break; }
        }
        std::cout << "Shader path: " << shaderPath << std::endl;
    }

    ShaderCreateInfo csCreateInfo;
    csCreateInfo.Stage = ShaderStage::Compute;
    csCreateInfo.SourcePath = shaderPath;
    csCreateInfo.EntryPoint = "CSMain";
    auto computeShader = compiler->CompileOrLoad(device, csCreateInfo);

    if (!computeShader)
    {
        std::cerr << "Failed to compile compute shader!" << std::endl;
        return 1;
    }
    std::cout << "Compute shader compiled!" << std::endl;

    // === 创建 Descriptor Set Layout ===
    // binding 0: Storage Image（compute shader 写入 Mandelbrot 结果）
    // binding 1: Uniform Buffer（分形参数：中心点、缩放、迭代次数等）
    auto descriptorSetLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::StorageImage, 1, ShaderStage::Compute)
        .AddBinding(1, DescriptorType::UniformBuffer, 1, ShaderStage::Compute)
        .Build());

    // === 创建 Compute Pipeline ===
    // Pipeline Layout 描述 shader 资源绑定布局
    // Compute Pipeline 绑定编译好的 compute shader + layout
    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder().AddSetLayout(descriptorSetLayout).Build());

    auto computePipeline = device->CreateComputePipeline(
        ComputePipelineBuilder()
        .SetShader(computeShader)
        .SetLayout(pipelineLayout)
        .Build());

    if (!computePipeline)
    {
        std::cerr << "Failed to create compute pipeline!" << std::endl;
        return 1;
    }
    std::cout << "Compute pipeline created!" << std::endl;

    // === 创建 Descriptor Pool + Set ===
    // 分配描述符集并绑定 Storage Image 和 Uniform Buffer
    auto descriptorPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder()
        .SetMaxSets(1)
        .AddPoolSize(DescriptorType::StorageImage, 1)
        .AddPoolSize(DescriptorType::UniformBuffer, 1)
        .Build());

    auto descriptorSet = descriptorPool->AllocateDescriptorSet(descriptorSetLayout);
    // binding 0: outputImage 作为 StorageImage（UAV），ResourceState::General 表示可读写
    descriptorSet->WriteTexture({0, outputImage->GetDefaultView(), ResourceState::General, DescriptorType::StorageImage, nullptr, 0});
    // binding 1: paramBuffer 作为 UniformBuffer
    descriptorSet->WriteBuffer({1, paramBuffer, 0, sizeof(MandelbrotParams), cbSize, DescriptorType::UniformBuffer, 0});
    descriptorSet->Update();

    std::cout << "Mandelbrot compute demo ready! Rendering..." << std::endl;

    // === 创建命令缓冲区（每帧一个）===
    std::vector<Ref<CommandBufferEncoder>> commandEncoders(framesInFlight);
    for (auto& e : commandEncoders)
        e = device->CreateCommandBufferEncoder(CommandBufferType::Primary);

    uint32_t frame = 0;
    MSG msg = {};
    bool running = true;

    // === 渲染主循环 ===
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        // 等待当前帧的 GPU 栅栏
        synchronization->WaitForFrame(frame);

        // 每帧更新分形参数（Map → 写入 → Unmap → Flush 到 GPU）
        MandelbrotParams params;
        params.width = rtW;
        params.height = rtH;
        params.centerX = g_centerX;
        params.centerY = g_centerY;
        params.zoom = g_zoom;
        params.maxIter = std::min(1000u, (uint32_t)(100 + g_zoom * 50));
        params._pad0 = 0;
        params._pad1 = 0;
        auto* pptr = static_cast<MandelbrotParams*>(paramBuffer->Map());
        *pptr = params;
        paramBuffer->Unmap();
        paramBuffer->Flush();

        // 获取下一张 swapchain 图像
        int imageIndex = 0;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
            continue;
        synchronization->ResetFrameFence(frame);

        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        if (backBuffer) backBuffer->CreateDefaultViewIfNeeded();

        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});

        // === 步骤 1: Dispatch Compute Shader ===
        // 绑定 compute pipeline → 绑定 descriptor sets → dispatch
        // 线程组大小 16x16，所以 dispatch (width/16, height/16, 1)
        cmd->BindComputePipeline(computePipeline);
        std::array<Ref<DescriptorSet>, 1> sets = {descriptorSet};
        cmd->BindComputeDescriptorSets(computePipeline, 0, sets);
        cmd->Dispatch((rtW + 15) / 16, (rtH + 15) / 16, 1);

        // === 步骤 2: 复制 compute 输出到 swapchain backbuffer ===
        // Barrier: outputImage UAV→CopySrc, backBuffer Present→CopyDst
        // CopyTexture2D/CopyResource: 全纹理复制
        // Barrier: outputImage CopySrc→UAV, backBuffer CopyDst→Present
#ifdef CACAO_USE_VULKAN
        // Vulkan 路径：使用跨平台 PipelineBarrier + CopyTexture2D
        {
            TextureBarrier barriers[2];
            barriers[0].Texture = outputImage;
            barriers[0].OldState = ResourceState::UnorderedAccess;
            barriers[0].NewState = ResourceState::CopySource;
            barriers[1].Texture = backBuffer;
            barriers[1].OldState = ResourceState::Present;
            barriers[1].NewState = ResourceState::CopyDest;
            cmd->PipelineBarrier(SyncScope::AllCommands, SyncScope::AllCommands, std::span<const TextureBarrier>(barriers, 2));
            cmd->CopyTexture2D(outputImage, backBuffer);
            barriers[0].OldState = ResourceState::CopySource;
            barriers[0].NewState = ResourceState::UnorderedAccess;
            barriers[1].OldState = ResourceState::CopyDest;
            barriers[1].NewState = ResourceState::Present;
            cmd->PipelineBarrier(SyncScope::AllCommands, SyncScope::AllCommands, std::span<const TextureBarrier>(barriers, 2));
        }
#else
        // DX12 路径：使用原生 D3D12 API（ResourceBarrier + CopyResource）
        cmd->ExecuteNative([&](void* nativeHandle) {
            auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(nativeHandle);
            auto* src = static_cast<D3D12Texture*>(outputImage.get())->GetHandle();
            auto* dst = static_cast<D3D12Texture*>(backBuffer.get())->GetHandle();
            D3D12_RESOURCE_BARRIER barriers[2] = {};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = src;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Transition.pResource = dst;
            barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            cmdList->ResourceBarrier(2, barriers);
            cmdList->CopyResource(dst, src);
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            cmdList->ResourceBarrier(2, barriers);
        });
#endif

        // === 步骤 3: 提交命令 + Present ===
        cmd->End();
        graphicsQueue->Submit(cmd, synchronization, frame);
        swapchain->Present(presentQueue, synchronization, frame);

        frame = (frame + 1) % framesInFlight;
    }

    // === 清理 ===
    synchronization->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    std::cout << "Compute demo finished." << std::endl;
    return 0;
}
