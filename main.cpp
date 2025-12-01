#include <fstream>
#include <windows.h>
#include <iostream>
#include "Cacao.hpp"
using namespace Cacao;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        return 0;
    case WM_SIZE:
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

std::vector<uint32_t> loadSpv(const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    return buffer;
}


int main()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const char* className = "CacaoWindowClass";
    const char* windowTitle = "Cacao Renderer";
    const int width = 800;
    const int height = 600;

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = className;
    wc.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

    if (!RegisterClassEx(&wc))
    {
        std::cerr << "Fatal Error: Failed to register window class." << std::endl;
        return EXIT_FAILURE;
    }

    RECT windowRect = {0, 0, width, height};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowEx(
        0,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
    {
        std::cerr << "Fatal Error: Failed to create window." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Window created successfully. Handles ready for Vulkan:" << std::endl;
    std::cout << " - HINSTANCE: " << hInstance << std::endl;
    std::cout << " - HWND: " << hWnd << std::endl;

    MSG msg = {};
    bool isRunning = true;
    CacaoInstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = CacaoType::Vulkan;
    instanceCreateInfo.applicationName = "CacaoApp";
    instanceCreateInfo.appVersion = 1;
    instanceCreateInfo.enabledFeatures = {CacaoInstanceFeature::ValidationLayer, CacaoInstanceFeature::Surface};
    Ref<CacaoInstance> instance = CacaoInstance::Create(instanceCreateInfo);
    if (!instance)
    {
        std::cerr << "Fatal Error: Failed to create Cacao instance." << std::endl;
        return EXIT_FAILURE;
    }
    Ref<CacaoAdapter> adapter = instance->EnumerateAdapters().front();
    if (!adapter)
    {
        std::cerr << "Fatal Error: No Cacao adapters found." << std::endl;
        return EXIT_FAILURE;
    }
    auto properties = adapter->GetProperties();
    std::cout << "Using adapter: " << properties.name << std::endl;

    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<CacaoSurface> surface = instance->CreateSurface(nativeWindowHandle);
    if (!surface)
    {
        std::cerr << "Fatal Error: Failed to create Cacao surface." << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<CacaoQueueRequest> queueRequests = {
        {QueueType::Graphics, 1, 1.0f},
    };
    CacaoDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = queueRequests;
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<CacaoDevice> device = adapter->CreateDevice(deviceCreateInfo);
    if (!device)
    {
        std::cerr << "Fatal Error: Failed to create Cacao device." << std::endl;
        return EXIT_FAILURE;
    }
    SwapchainCreateInfo swapchainCreateInfo;
    auto surfaceCaps = surface->GetCapabilities(adapter);
    swapchainCreateInfo.Extent = surfaceCaps.currentExtent;
    swapchainCreateInfo.Format = Format::BGRA8_UNORM;
    swapchainCreateInfo.ColorSpace = ColorSpace::SRGB_NONLINEAR;
    swapchainCreateInfo.PresentMode = PresentMode::Mailbox;
    swapchainCreateInfo.MinImageCount = surfaceCaps.minImageCount + 1;
    if (swapchainCreateInfo.MinImageCount > surfaceCaps.maxImageCount &&
        surfaceCaps.maxImageCount != 0)
    {
        swapchainCreateInfo.MinImageCount = surfaceCaps.maxImageCount;
    }
    swapchainCreateInfo.CompatibleSurface = surface;
    swapchainCreateInfo.PreTransform = surfaceCaps.currentTransform;
    swapchainCreateInfo.Clipped = true;

    Ref<CacaoSwapchain> swapchain = device->CreateSwapchain(swapchainCreateInfo);
    if (!swapchain)
    {
        std::cerr << "Fatal Error: Failed to create Cacao swapchain." << std::endl;
        return EXIT_FAILURE;
    }

    Ref<CacaoSynchronization> synchronization = device->CreateSynchronization(swapchain->GetImageCount());
    if (!synchronization)
    {
        std::cerr << "Fatal Error: Failed to create Cacao synchronization objects." << std::endl;
        return EXIT_FAILURE;
    }
    auto compiler = instance->CreateShaderCompiler();
    if (!compiler)
    {
        std::cerr << "Fatal Error: Failed to create Cacao shader compiler." << std::endl;
        return EXIT_FAILURE;
    }
    // ...  着色器编译部分 ...
    ShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.Stage = ShaderStage::Vertex;
    shaderCreateInfo.SourcePath = "test.slang";
    shaderCreateInfo.EntryPoint = "mainVS";
    auto vertexShaderModule = compiler->CompileOrLoad(device, shaderCreateInfo);
    if (!vertexShaderModule)
    {
        std::cerr << "Fatal Error: Failed to create vertex shader module." << std::endl;
        return EXIT_FAILURE;
    }

    shaderCreateInfo.Stage = ShaderStage::Fragment;
    shaderCreateInfo.SourcePath = "test.slang";
    shaderCreateInfo.EntryPoint = "mainPS";
    auto fragmentShaderModule = compiler->CompileOrLoad(device, shaderCreateInfo);
    if (!fragmentShaderModule)
    {
        std::cerr << "Fatal Error: Failed to create fragment shader module." << std::endl;
        return EXIT_FAILURE;
    }

    // === 创建管线布局 ===
    PipelineLayoutCreateInfo layoutCreateInfo;
    // 如果没有描述符集和推送常量，可以留空
    // layoutCreateInfo. DescriptorSetLayouts = {};
    // layoutCreateInfo.PushConstantRanges = {};
    auto pipelineLayout = device->CreatePipelineLayout(layoutCreateInfo);

    // === 创建图形管线 ===
    GraphicsPipelineCreateInfo pipelineCreateInfo;

    // 1. 着色器
    pipelineCreateInfo.Shaders = {vertexShaderModule, fragmentShaderModule};

    // 2. 顶点输入（无顶点缓冲区，使用着色器内置数组）
    pipelineCreateInfo.VertexBindings = {};
    pipelineCreateInfo.VertexAttributes = {};

    // 3. 输入汇编
    pipelineCreateInfo.InputAssembly.Topology = PrimitiveTopology::TriangleList;
    pipelineCreateInfo.InputAssembly.PrimitiveRestartEnable = false;

    // 4. 光栅化状态
    pipelineCreateInfo.Rasterizer.PolygonMode = PolygonMode::Fill;
    pipelineCreateInfo.Rasterizer.CullMode = CullMode::Back;
    pipelineCreateInfo.Rasterizer.FrontFace = FrontFace::Clockwise;
    pipelineCreateInfo.Rasterizer.LineWidth = 1.0f;
    pipelineCreateInfo.Rasterizer.DepthClampEnable = false;
    pipelineCreateInfo.Rasterizer.RasterizerDiscardEnable = false;
    pipelineCreateInfo.Rasterizer.DepthBiasEnable = false;

    // 5. 多重采样状态
    pipelineCreateInfo.Multisample.RasterizationSamples = 1;
    pipelineCreateInfo.Multisample.SampleShadingEnable = false;
    pipelineCreateInfo.Multisample.AlphaToCoverageEnable = false;
    pipelineCreateInfo.Multisample.AlphaToOneEnable = false;

    // 6. 深度模板状态（禁用深度测试）
    pipelineCreateInfo.DepthStencil.DepthTestEnable = false;
    pipelineCreateInfo.DepthStencil.DepthWriteEnable = false;
    pipelineCreateInfo.DepthStencil.StencilTestEnable = false;

    // 7. 颜色混合状态
    ColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.BlendEnable = false;
    colorBlendAttachment.ColorWriteMask = ColorComponentFlags::All;
    pipelineCreateInfo.ColorBlend.Attachments = {colorBlendAttachment};
    pipelineCreateInfo.ColorBlend.LogicOpEnable = false;

    // 8. 动态渲染格式（根据交换链格式设置）
    pipelineCreateInfo.ColorAttachmentFormats = {swapchain->GetFormat()}; // 或 Format::BGRA8_UNORM
    pipelineCreateInfo.DepthStencilFormat = Format::UNDEFINED; // 不使用深度缓冲

    // 9. 管线布局
    pipelineCreateInfo.Layout = pipelineLayout;

    // 10.  管线缓存
    pipelineCreateInfo.Cache = nullptr;

    // 创建图形管线
    auto graphicsPipeline = device->CreateGraphicsPipeline(pipelineCreateInfo);
    if (!graphicsPipeline)
    {
        std::cerr << "Fatal Error: Failed to create graphics pipeline." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Graphics pipeline created successfully!" << std::endl;
    std::vector<Ref<CacaoCommandBufferEncoder>> commandEncoders(swapchain->GetImageCount());
    for (uint32_t i = 0; i < commandEncoders.size(); i++)
    {
        commandEncoders[i] = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
    }

    auto presentQueue = surface->GetPresentQueue(device);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    int frame = 0;
    while (isRunning)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) isRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        synchronization->WaitForFrame(frame);
        int imageIndex;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
        {
            continue;
        }
        synchronization->ResetFrameFence(frame);
        auto texture = swapchain->GetBackBuffer(imageIndex);
        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});
        TextureBarrier barrier;
        barrier.Texture = texture;
        barrier.OldLayout = ImageLayout::Undefined;
        barrier.NewLayout = ImageLayout::ColorAttachment;
        barrier.SrcAccess = AccessFlags::None;
        barrier.DstAccess = AccessFlags::ColorAttachmentWrite;
        barrier.SubresourceRange.LayerCount = 1;
        barrier.SubresourceRange.LevelCount = 1;
        barrier.SubresourceRange.AspectMask = ImageAspectFlags::Color;
        cmd->PipelineBarrier(
            PipelineStage::TopOfPipe,
            PipelineStage::ColorAttachmentOutput,
            {barrier}
        );
        RenderingAttachmentInfo colorAttachment;
        colorAttachment.Texture = texture;
        colorAttachment.LoadOp = AttachmentLoadOp::Clear;
        colorAttachment.StoreOp = AttachmentStoreOp::Store;
        colorAttachment.ClearValue = ClearValue({0.f, 0.f, 0.f, 1.f});
        RenderingInfo renderInfo;
        renderInfo.RenderArea = {0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height};
        renderInfo.LayerCount = 1;
        renderInfo.ColorAttachments = {colorAttachment};
        cmd->BeginRendering(renderInfo);
        cmd->SetViewport({
            0.0f, 0.0f,
            static_cast<float>(swapchain->GetExtent().width),
            static_cast<float>(swapchain->GetExtent().height),
            0.0f, 1.0f
        });
        cmd->SetScissor({0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height});
        cmd->BindGraphicsPipeline(graphicsPipeline);
        cmd->Draw(3, 1, 0, 0);
        cmd->EndRendering();
        TextureBarrier barrier2;
        barrier2.Texture = texture;
        barrier2.OldLayout = ImageLayout::ColorAttachment;
        barrier2.NewLayout = ImageLayout::Present;
        barrier2.SrcAccess = AccessFlags::ColorAttachmentWrite;
        barrier2.DstAccess = AccessFlags::None;
        barrier2.SubresourceRange.LayerCount = 1;
        barrier2.SubresourceRange.LevelCount = 1;
        barrier2.SubresourceRange.AspectMask = ImageAspectFlags::Color;
        cmd->PipelineBarrier(
            PipelineStage::ColorAttachmentOutput,
            PipelineStage::BottomOfPipe,
            {barrier2}
        );
        cmd->End();

        graphicsQueue->Submit(cmd, synchronization, frame);

        swapchain->Present(presentQueue, synchronization, frame);
        frame = (frame + 1) % commandEncoders.size();
    }
    device->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    return EXIT_SUCCESS;
}
