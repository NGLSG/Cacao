#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <filesystem>
#include "Cacao.hpp"

using namespace Cacao;

// 顶点结构：二维位置 + 二维纹理坐标
struct Vertex
{
    float pos[2];
    float uv[2];
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    // 设置 DPI 感知，避免高 DPI 显示器上窗口大小不正确
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const char* className = "TexturedQuadClass";
    BackendType selectedBackend = BackendType::DirectX12;

    // === 创建 Win32 窗口 ===
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

    RECT windowRect = {0, 0, 800, 600};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(0, className, "Cacao - Textured Quad",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    std::cout << "=== Cacao Textured Quad ===" << std::endl;

    // === 初始化 Cacao RHI ===
    // 创建实例：选择后端类型，启用验证层和窗口 Surface 支持
    InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = selectedBackend;
    instanceCreateInfo.applicationName = "TexturedQuad";
    instanceCreateInfo.appVersion = 1;
    instanceCreateInfo.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    Ref<Instance> instance = Instance::Create(instanceCreateInfo);

    // 枚举 GPU 适配器，选第一个
    Ref<Adapter> adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    // 创建窗口 Surface（连接窗口和 GPU）
    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<Surface> surface = instance->CreateSurface(nativeWindowHandle);

    // 创建逻辑设备：请求一个图形队列
    DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<Device> device = adapter->CreateDevice(deviceCreateInfo);

    // === 创建交换链 ===
    // 交换链管理多个后缓冲区，实现双缓冲/三缓冲渲染
    auto surfaceCaps = surface->GetCapabilities(adapter);
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (imageCount > surfaceCaps.maxImageCount && surfaceCaps.maxImageCount != 0)
        imageCount = surfaceCaps.maxImageCount;

    Ref<Swapchain> swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(surfaceCaps.currentExtent)
        .SetFormat(Format::BGRA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo)      // 垂直同步
        .SetMinImageCount(imageCount)
        .SetSurface(surface)
        .SetPreTransform(surfaceCaps.currentTransform)
        .SetClipped(true)
        .Build());

    // 创建帧同步对象（信号量 + 栅栏），管理 CPU-GPU 同步
    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    Ref<Synchronization> synchronization = device->CreateSynchronization(framesInFlight);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    // === 创建四边形几何体 ===
    // 4 个顶点 + 6 个索引 = 2 个三角形组成的矩形
    Vertex quadVerts[] = {
        {{-0.8f, -0.8f}, {0.0f, 1.0f}},   // 左下角，UV 左下
        {{ 0.8f, -0.8f}, {1.0f, 1.0f}},   // 右下角，UV 右下
        {{ 0.8f,  0.8f}, {1.0f, 0.0f}},   // 右上角，UV 右上
        {{-0.8f,  0.8f}, {0.0f, 0.0f}},   // 左上角，UV 左上
    };
    uint32_t quadIndices[] = {0, 1, 2, 0, 2, 3};  // 两个三角形

    // 创建顶点缓冲区并上传数据
    auto vb = device->CreateBuffer(BufferBuilder()
        .SetSize(sizeof(quadVerts))
        .SetUsage(BufferUsageFlags::VertexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu) // CPU 可写，GPU 可读
        .SetName("QuadVB")
        .Build());
    memcpy(vb->Map(), quadVerts, sizeof(quadVerts));
    vb->Unmap();

    // 创建索引缓冲区并上传数据
    auto ib = device->CreateBuffer(BufferBuilder()
        .SetSize(sizeof(quadIndices))
        .SetUsage(BufferUsageFlags::IndexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("QuadIB")
        .Build());
    memcpy(ib->Map(), quadIndices, sizeof(quadIndices));
    ib->Unmap();

    // === 创建棋盘纹理 ===
    // 在 CPU 端生成 64x64 的棋盘图案像素数据（RGBA8 格式）
    const uint32_t texSize = 64;
    std::vector<uint8_t> pixels(texSize * texSize * 4);
    for (uint32_t y = 0; y < texSize; y++)
    {
        for (uint32_t x = 0; x < texSize; x++)
        {
            bool white = ((x / 8) + (y / 8)) % 2 == 0; // 每 8 像素一格
            uint32_t idx = (y * texSize + x) * 4;
            pixels[idx + 0] = white ? 240 : 40;   // R
            pixels[idx + 1] = white ? 220 : 60;   // G
            pixels[idx + 2] = white ? 200 : 80;   // B
            pixels[idx + 3] = 255;                  // A（不透明）
        }
    }

    // 创建 GPU 纹理对象
    auto texture = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(texSize, texSize)
        .SetMipLevels(1)
        .SetArrayLayers(1)
        .SetFormat(Format::RGBA8_UNORM)
        .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
            // Sampled = 可在 shader 中采样；TransferDst = 可作为拷贝目标
        .SetName("Checkerboard")
        .Build());

    // 创建临时 staging 缓冲区，用于从 CPU 向 GPU 纹理上传数据
    // GPU 纹理通常位于显存，CPU 不能直接写入，需要先写入 staging buffer 再拷贝
    auto stagingBuffer = device->CreateBuffer(
        BufferBuilder()
        .SetSize(pixels.size())
        .SetUsage(BufferUsageFlags::TransferSrc) // 只作为拷贝源
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .Build());
    memcpy(stagingBuffer->Map(), pixels.data(), pixels.size());
    stagingBuffer->Flush(); // 确保 CPU 写入刷新到 GPU 可见

    // 录制并提交纹理上传命令
    {
        auto uploadCmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
        uploadCmd->Reset();
        uploadCmd->Begin({true});

        // 将纹理从"未定义"状态转换为"可接收拷贝"状态
        uploadCmd->TransitionImage(texture, ImageTransition::UndefinedToTransferDst);

        // 指定拷贝区域：整个纹理
        BufferImageCopy region = {};
        region.BufferOffset = 0;
        region.BufferRowLength = 0;         // 0 = 紧密排列（无行对齐填充）
        region.BufferImageHeight = 0;
        region.ImageSubresource.AspectMask = ImageAspectFlags::Color;
        region.ImageSubresource.MipLevel = 0;
        region.ImageSubresource.BaseArrayLayer = 0;
        region.ImageSubresource.LayerCount = 1;
        region.ImageExtentWidth = texSize;
        region.ImageExtentHeight = texSize;
        region.ImageExtentDepth = 1;

        // 执行 buffer → texture 拷贝
        uploadCmd->CopyBufferToImage(stagingBuffer, texture, ResourceState::CopyDest, {&region, 1});

        // 将纹理从"拷贝目标"转换为"可在 shader 中采样"状态
        uploadCmd->TransitionImage(texture, ImageTransition::TransferDstToShaderRead);
        uploadCmd->End();

        // 提交并等待 GPU 完成上传
        graphicsQueue->Submit(uploadCmd);
        graphicsQueue->WaitIdle();
    }
    // 为纹理创建默认视图（shader 访问纹理需要通过视图）
    texture->CreateDefaultViewIfNeeded();

    // === 创建采样器 ===
    // 采样器定义纹理采样时的过滤和寻址方式
    auto sampler = device->CreateSampler(
        SamplerBuilder()
        .SetFilter(Filter::Linear, Filter::Linear) // 双线性过滤
        .SetAddressMode(SamplerAddressMode::Repeat) // UV 超出 [0,1] 时重复
        .SetAnisotropy(false)
        .Build());

    // === 编译着色器 ===
    // 清理旧缓存，确保使用最新 shader 源码
    {
        namespace fs = std::filesystem;
        if (fs::exists("shader_cache_texquad"))
            fs::remove_all("shader_cache_texquad");
    }

    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_texquad");

    // 查找 shader 文件路径
    std::string shaderPath = "textured_quad.slang";
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        auto sep = exeDir.find_last_of("\\/");
        if (sep != std::string::npos) exeDir = exeDir.substr(0, sep + 1);
        std::string candidates[] = {
            exeDir + "textured_quad.slang",
            exeDir + "..\\demos\\textured_quad.slang",
            exeDir + "..\\..\\demos\\textured_quad.slang",
            "demos\\textured_quad.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\textured_quad.slang"
        };
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) { shaderPath = c; break; }
        }
        std::cout << "Shader path: " << shaderPath << std::endl;
    }

    // 编译顶点着色器
    ShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.Stage = ShaderStage::Vertex;
    shaderCreateInfo.SourcePath = shaderPath;
    shaderCreateInfo.EntryPoint = "mainVS";
    auto vertexShader = compiler->CompileOrLoad(device, shaderCreateInfo);

    // 编译片段（像素）着色器
    shaderCreateInfo.Stage = ShaderStage::Fragment;
    shaderCreateInfo.EntryPoint = "mainPS";
    auto fragmentShader = compiler->CompileOrLoad(device, shaderCreateInfo);

    if (!vertexShader || !fragmentShader)
    {
        std::cerr << "Failed to compile shaders!" << std::endl;
        return 1;
    }
    std::cout << "Shaders compiled!" << std::endl;

    // === 创建描述符集布局 ===
    // 描述符集布局定义 shader 可以访问哪些资源及其绑定点
    auto descriptorSetLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::SampledImage, 1, ShaderStage::Fragment) // 纹理
        .AddBinding(1, DescriptorType::Sampler, 1, ShaderStage::Fragment)      // 采样器
        .Build());

    // 管线布局：告诉管线使用哪些描述符集布局
    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder().AddSetLayout(descriptorSetLayout).Build());

    // === 创建图形管线 ===
    // 图形管线定义整个渲染流程：输入布局、着色器、光栅化设置、混合模式等
    auto graphicsPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .SetShaders({vertexShader, fragmentShader})
        .AddVertexBinding(0, sizeof(Vertex)) // 顶点缓冲区绑定：步长 = Vertex 大小
        .AddVertexAttribute(0, 0, Format::RG32_FLOAT, offsetof(Vertex, pos), "POSITION", 0)
            // location=0, 对应 shader 的 POSITION 语义
        .AddVertexAttribute(1, 0, Format::RG32_FLOAT, offsetof(Vertex, uv), "TEXCOORD", 0)
            // location=1, 对应 shader 的 TEXCOORD 语义
        .SetTopology(PrimitiveTopology::TriangleList)
        .SetPolygonMode(PolygonMode::Fill)
        .SetCullMode(CullMode::None)                    // 不剔除任何面
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetLineWidth(1.0f)
        .SetSampleCount(1)
        .SetDepthTest(false, false)                      // 2D 渲染不需要深度测试
        .AddColorAttachmentDefault(false)                // 不需要 alpha 混合
        .AddColorFormat(swapchain->GetFormat())          // 颜色格式与交换链一致
        .SetLayout(pipelineLayout)
        .Build());

    if (!graphicsPipeline)
    {
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
        return 1;
    }
    std::cout << "Pipeline created!" << std::endl;

    // === 创建描述符集并绑定资源 ===
    // 描述符池分配描述符集，描述符集将实际资源绑定到 shader 的绑定点
    auto descriptorPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder()
        .SetMaxSets(1)
        .AddPoolSize(DescriptorType::SampledImage, 1)
        .AddPoolSize(DescriptorType::Sampler, 1)
        .Build());
    auto descriptorSet = descriptorPool->AllocateDescriptorSet(descriptorSetLayout);

    // 绑定纹理到 binding=0
    descriptorSet->WriteTexture({
        0, texture->GetDefaultView(), ResourceState::ShaderRead, DescriptorType::SampledImage, nullptr, 0
    });
    // 绑定采样器到 binding=1
    descriptorSet->WriteSampler({1, sampler, 0});
    // 提交绑定更新
    descriptorSet->Update();

    // === 创建命令缓冲区编码器 ===
    // 每帧一个命令缓冲区，避免 CPU/GPU 竞争
    std::vector<Ref<CommandBufferEncoder>> commandEncoders(framesInFlight);
    for (auto& e : commandEncoders)
        e = device->CreateCommandBufferEncoder(CommandBufferType::Primary);

    // 跟踪交换链图像是否已初始化（首次使用需要特殊转换）
    std::vector<bool> swapchainImageInitialized(swapchain->GetImageCount(), false);
    uint32_t frame = 0;
    MSG msg = {};
    bool running = true;

    std::cout << "Textured Quad ready! Rendering..." << std::endl;

    // === 主渲染循环 ===
    while (running)
    {
        // 处理 Windows 消息
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        // 等待当前帧的 GPU 工作完成（避免覆盖正在使用的资源）
        synchronization->WaitForFrame(frame);

        // 获取下一个可用的交换链图像
        int imageIndex = 0;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
            continue;
        synchronization->ResetFrameFence(frame);

        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        if (backBuffer) backBuffer->CreateDefaultViewIfNeeded();

        // 开始录制命令
        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});

        // 转换后缓冲区状态为"颜色附件"（可渲染）
        if (!swapchainImageInitialized[imageIndex])
        {
            cmd->TransitionImage(backBuffer, ImageTransition::UndefinedToColorAttachment);
            swapchainImageInitialized[imageIndex] = true;
        }
        else
        {
            cmd->TransitionImage(backBuffer, ImageTransition::PresentToColorAttachment);
        }

        // 设置渲染目标（动态渲染，不需要 Render Pass 对象）
        RenderingAttachmentInfo colorAttachment;
        colorAttachment.Texture = backBuffer;
        colorAttachment.LoadOp = AttachmentLoadOp::Clear;    // 渲染前清除
        colorAttachment.StoreOp = AttachmentStoreOp::Store;  // 渲染后保存
        colorAttachment.ClearValue = ClearValue::ColorFloat(0.1f, 0.1f, 0.12f, 1.0f);

        RenderingInfo renderInfo;
        renderInfo.RenderArea = {0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height};
        renderInfo.LayerCount = 1;
        renderInfo.ColorAttachments = {colorAttachment};

        cmd->BeginRendering(renderInfo);

        // 设置视口和裁剪矩形
        cmd->SetViewport({
            0.0f, 0.0f,
            static_cast<float>(swapchain->GetExtent().width),
            static_cast<float>(swapchain->GetExtent().height),
            0.0f, 1.0f
        });
        cmd->SetScissor({0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height});

        // 绑定管线、顶点/索引缓冲区、描述符集，然后绘制
        cmd->BindGraphicsPipeline(graphicsPipeline);
        cmd->BindVertexBuffer(0, vb, 0);
        cmd->BindIndexBuffer(ib, 0, IndexType::UInt32);
        std::array<Ref<DescriptorSet>, 1> sets = {descriptorSet};
        cmd->BindDescriptorSets(graphicsPipeline, 0, sets);
        cmd->DrawIndexed(6, 1, 0, 0, 0); // 6 个索引，1 个实例

        cmd->EndRendering();

        // 转换后缓冲区状态为"可呈现"
        cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
        cmd->End();

        // 提交命令并呈现
        graphicsQueue->Submit(cmd, synchronization, frame);
        swapchain->Present(presentQueue, synchronization, frame);

        frame = (frame + 1) % framesInFlight;
    }

    // 等待所有 GPU 工作完成后再销毁资源
    synchronization->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    std::cout << "Textured Quad finished." << std::endl;
    return 0;
}
