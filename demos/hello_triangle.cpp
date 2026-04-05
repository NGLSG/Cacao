// ============================================================
// Hello Triangle - Cacao RHI 入门示例
//
// 展示使用 Cacao 图形抽象层渲染一个彩色三角形的完整流程：
// 1. Win32 窗口创建
// 2. Cacao 实例、适配器、设备初始化
// 3. Swapchain 创建与配置
// 4. 顶点缓冲区、着色器编译、图形管线创建
// 5. 帧同步与渲染循环
// 6. 资源清理
//
// 使用 DX12 后端，不依赖任何特定后端代码。
// ============================================================

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <filesystem>
#include "Cacao.hpp"

using namespace Cacao;

// 顶点结构：2D 位置 + RGB 颜色
// 布局必须与着色器的 VSInput 和管线的 VertexAttribute 一致
struct Vertex
{
    float pos[2];    // NDC 空间位置 (-1 到 1)
    float color[3];  // RGB 颜色 (0 到 1)
};

// Win32 窗口消息处理：关闭窗口或按 ESC 退出
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    // 启用 Per-Monitor DPI 感知，避免 Windows 自动缩放导致模糊
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const char* className = "HelloTriangleClass";
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

    // AdjustWindowRect 将客户区大小转换为包含标题栏/边框的窗口大小
    RECT windowRect = {0, 0, 800, 600};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(0, className, "Cacao - Hello Triangle",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    std::cout << "=== Cacao Hello Triangle ===" << std::endl;

    // === 初始化 Cacao 图形实例 ===
    // Instance 是 Cacao 的入口点，管理后端初始化和全局状态
    InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = selectedBackend;
    instanceCreateInfo.applicationName = "HelloTriangle";
    instanceCreateInfo.appVersion = 1;
    // ValidationLayer: 启用 GPU 验证层，开发时帮助发现错误
    // Surface: 启用窗口表面支持，用于 swapchain 呈现
    instanceCreateInfo.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    Ref<Instance> instance = Instance::Create(instanceCreateInfo);

    // 枚举 GPU 适配器，选择第一个（通常是独立显卡）
    Ref<Adapter> adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    // 创建窗口表面：连接 Win32 窗口和图形后端
    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<Surface> surface = instance->CreateSurface(nativeWindowHandle);

    // === 创建逻辑设备 ===
    // 请求一个图形队列，优先级 1.0（最高）
    // CompatibleSurface 确保设备支持向该表面呈现
    DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<Device> device = adapter->CreateDevice(deviceCreateInfo);

    // === 创建 Swapchain ===
    // Swapchain 管理多个后缓冲区，实现双缓冲/三缓冲
    auto surfaceCaps = surface->GetCapabilities(adapter);
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (imageCount > surfaceCaps.maxImageCount && surfaceCaps.maxImageCount != 0)
        imageCount = surfaceCaps.maxImageCount;

    Ref<Swapchain> swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(surfaceCaps.currentExtent)   // 使用表面报告的当前尺寸
        .SetFormat(Format::BGRA8_UNORM)         // 8位 BGRA 格式（D3D12 常用）
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR) // sRGB 色彩空间
        .SetPresentMode(PresentMode::Fifo)      // 垂直同步（等待显示器刷新）
        .SetMinImageCount(imageCount)
        .SetSurface(surface)
        .SetPreTransform(surfaceCaps.currentTransform)
        .SetClipped(true)                       // 裁剪被遮挡的像素
        .Build());

    // === 帧同步与队列 ===
    // framesInFlight: CPU 可以提前准备的帧数（通常 2 帧）
    // Synchronization 管理 CPU-GPU 同步的 fence 和 semaphore
    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    Ref<Synchronization> synchronization = device->CreateSynchronization(framesInFlight);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    // === 顶点数据 ===
    // 三个顶点构成一个三角形，每个顶点有不同颜色
    // GPU 会在三角形内部自动插值颜色，产生渐变效果
    Vertex triangleVerts[] = {
        {{ 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}},  // 顶部 - 红色
        {{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // 左下 - 绿色
        {{ 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},  // 右下 - 蓝色
    };

    // 创建顶点缓冲区：CPU 可写 (CpuToGpu)，用作顶点输入 (VertexBuffer)
    auto vb = device->CreateBuffer(BufferBuilder()
        .SetSize(sizeof(triangleVerts))
        .SetUsage(BufferUsageFlags::VertexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("TriangleVB")
        .Build());
    // Map/Unmap: 获取 CPU 可写指针，拷贝顶点数据到 GPU 可见内存
    memcpy(vb->Map(), triangleVerts, sizeof(triangleVerts));
    vb->Unmap();

    // === 编译着色器 ===
    // ShaderCompiler 将 Slang 源码编译为后端特定的字节码 (DXIL/SPIRV)
    // 编译结果可缓存到磁盘，避免重复编译
    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_triangle");

    // 解析 shader 文件路径：依次尝试多个可能的位置
    std::string shaderPath = "hello_triangle.slang";
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        auto sep = exeDir.find_last_of("\\/");
        if (sep != std::string::npos) exeDir = exeDir.substr(0, sep + 1);
        std::string candidates[] = {
            exeDir + "hello_triangle.slang",
            exeDir + "..\\demos\\hello_triangle.slang",
            exeDir + "..\\..\\demos\\hello_triangle.slang",
            "demos\\hello_triangle.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\hello_triangle.slang"
        };
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) { shaderPath = c; break; }
        }
        std::cout << "Shader path: " << shaderPath << std::endl;
    }

    // 编译顶点着色器和片段着色器
    // 同一个 .slang 文件可包含多个入口点
    ShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.Stage = ShaderStage::Vertex;
    shaderCreateInfo.SourcePath = shaderPath;
    shaderCreateInfo.EntryPoint = "mainVS";
    auto vertexShader = compiler->CompileOrLoad(device, shaderCreateInfo);

    shaderCreateInfo.Stage = ShaderStage::Fragment;
    shaderCreateInfo.EntryPoint = "mainPS";
    auto fragmentShader = compiler->CompileOrLoad(device, shaderCreateInfo);

    if (!vertexShader || !fragmentShader)
    {
        std::cerr << "Failed to compile shaders!" << std::endl;
        return 1;
    }
    std::cout << "Shaders compiled!" << std::endl;

    // === 创建管线布局和图形管线 ===
    // PipelineLayout: 描述管线使用的资源绑定（这里为空，三角形不需要额外资源）
    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder().Build());

    // GraphicsPipeline: 完整的渲染管线状态对象
    // 包含着色器、顶点格式、光栅化设置、混合模式等
    auto graphicsPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .SetShaders({vertexShader, fragmentShader})
        // 顶点布局：绑定点0，步长=sizeof(Vertex)
        .AddVertexBinding(0, sizeof(Vertex))
        // 位置属性：location=0, binding=0, float2, 偏移=0, 语义=POSITION
        .AddVertexAttribute(0, 0, Format::RG32_FLOAT, offsetof(Vertex, pos), "POSITION", 0)
        // 颜色属性：location=1, binding=0, float3, 偏移=8, 语义=COLOR
        .AddVertexAttribute(1, 0, Format::RGB32_FLOAT, offsetof(Vertex, color), "COLOR", 0)
        .SetTopology(PrimitiveTopology::TriangleList)  // 每3个顶点构成一个三角形
        .SetPolygonMode(PolygonMode::Fill)              // 填充模式（不是线框）
        .SetCullMode(CullMode::None)                    // 不剔除背面（双面可见）
        .SetFrontFace(FrontFace::CounterClockwise)      // 逆时针为正面
        .SetLineWidth(1.0f)
        .SetSampleCount(1)                              // 不使用多重采样
        .SetDepthTest(false, false)                     // 不使用深度测试
        .AddColorAttachmentDefault(false)               // 默认颜色混合（不透明）
        .AddColorFormat(swapchain->GetFormat())         // 渲染目标格式必须匹配 swapchain
        .SetLayout(pipelineLayout)
        .Build());

    if (!graphicsPipeline)
    {
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
        return 1;
    }
    std::cout << "Pipeline created!" << std::endl;

    // === 创建命令编码器 ===
    // 每帧一个命令编码器，避免 CPU-GPU 争用
    std::vector<Ref<CommandBufferEncoder>> commandEncoders(framesInFlight);
    for (auto& e : commandEncoders)
        e = device->CreateCommandBufferEncoder(CommandBufferType::Primary);

    // 跟踪 swapchain 图像是否已初始化（第一次使用需要特殊转换）
    std::vector<bool> swapchainImageInitialized(swapchain->GetImageCount(), false);
    uint32_t frame = 0;
    MSG msg = {};
    bool running = true;

    std::cout << "Hello Triangle ready! Rendering..." << std::endl;

    // === 渲染循环 ===
    while (running)
    {
        // 处理 Windows 消息（键盘、鼠标、窗口关闭等）
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        // 等待当前帧的 GPU 工作完成（避免覆盖正在使用的资源）
        synchronization->WaitForFrame(frame);

        // 获取下一个可用的 swapchain 图像
        int imageIndex = 0;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
            continue;
        // 重置帧 fence，为新的 GPU 提交做准备
        synchronization->ResetFrameFence(frame);

        // 获取后缓冲区纹理，确保有默认视图（用于渲染目标绑定）
        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        if (backBuffer) backBuffer->CreateDefaultViewIfNeeded();

        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});  // true = 一次性提交（不重用）

        // 资源屏障：将后缓冲区从 Present/Undefined 状态转换为 ColorAttachment
        // GPU 需要知道资源的当前用途，以正确处理缓存和同步
        if (!swapchainImageInitialized[imageIndex])
        {
            cmd->TransitionImage(backBuffer, ImageTransition::UndefinedToColorAttachment);
            swapchainImageInitialized[imageIndex] = true;
        }
        else
        {
            cmd->TransitionImage(backBuffer, ImageTransition::PresentToColorAttachment);
        }

        // 配置渲染附件：使用后缓冲区作为颜色输出，每帧清除为深灰色
        RenderingAttachmentInfo colorAttachment;
        colorAttachment.Texture = backBuffer;
        colorAttachment.LoadOp = AttachmentLoadOp::Clear;   // 渲染前清除
        colorAttachment.StoreOp = AttachmentStoreOp::Store;  // 渲染后保存结果
        colorAttachment.ClearValue = ClearValue::ColorFloat(0.1f, 0.1f, 0.12f, 1.0f);

        // 开始动态渲染通道（不使用预创建的 RenderPass 对象）
        RenderingInfo renderInfo;
        renderInfo.RenderArea = {0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height};
        renderInfo.LayerCount = 1;
        renderInfo.ColorAttachments = {colorAttachment};

        cmd->BeginRendering(renderInfo);

        // 设置视口和裁剪矩形（覆盖整个窗口）
        cmd->SetViewport({
            0.0f, 0.0f,
            static_cast<float>(swapchain->GetExtent().width),
            static_cast<float>(swapchain->GetExtent().height),
            0.0f, 1.0f
        });
        cmd->SetScissor({0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height});

        // 绑定管线和顶点缓冲区，发出绘制命令
        cmd->BindGraphicsPipeline(graphicsPipeline);
        cmd->BindVertexBuffer(0, vb, 0);   // 绑定点0，偏移0
        cmd->Draw(3, 1, 0, 0);             // 3个顶点，1个实例

        cmd->EndRendering();

        // 渲染完成后，将后缓冲区转换为 Present 状态，准备显示
        cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
        cmd->End();

        // 提交命令到 GPU 队列，附带帧同步信息
        graphicsQueue->Submit(cmd, synchronization, frame);
        // 呈现：将渲染结果显示到窗口
        swapchain->Present(presentQueue, synchronization, frame);

        // 循环使用帧索引（在 framesInFlight 之间轮转）
        frame = (frame + 1) % framesInFlight;
    }

    // === 清理 ===
    // 等待所有 GPU 工作完成后再销毁资源
    synchronization->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    std::cout << "Hello Triangle finished." << std::endl;
    return 0;
}
