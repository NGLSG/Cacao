#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include "Cacao.hpp"

using namespace Cacao;

// 顶点结构：位置 + 颜色，与 Slang 着色器的 VSInput 布局一致
struct Vertex
{
    float pos[3];   // 模型空间坐标
    float color[3]; // RGB 颜色
};

// MVP 矩阵数据，通过 Uniform Buffer 传给着色器
struct MVPData
{
    float mvp[16]; // 4x4 列主序矩阵（Model * View * Projection）
};

// ============ 矩阵工具函数 ============

// 生成 4x4 单位矩阵（列主序，float[16]）
static void MatIdentity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// 4x4 矩阵乘法（列主序）：out = a * b
static void MatMul(float* out, const float* a, const float* b)
{
    float tmp[16];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
        {
            tmp[c * 4 + r] = 0;
            for (int k = 0; k < 4; k++)
                tmp[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
        }
    memcpy(out, tmp, 64);
}

// 绕 Y 轴旋转矩阵
static void MatRotateY(float* m, float angle)
{
    MatIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;  m[8] = s;
    m[2] = -s; m[10] = c;
}

// 绕 X 轴旋转矩阵
static void MatRotateX(float* m, float angle)
{
    MatIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m[5] = c;  m[9] = -s;
    m[6] = s;  m[10] = c;
}

// 透视投影矩阵（Reversed-Z 兼容，近平面 zn，远平面 zf）
static void MatPerspective(float* m, float fovY, float aspect, float zn, float zf)
{
    float tanHalf = tanf(fovY * 0.5f);
    memset(m, 0, 64);
    m[0] = 1.0f / (aspect * tanHalf);  // X 缩放（考虑宽高比）
    m[5] = 1.0f / tanHalf;             // Y 缩放
    m[10] = zf / (zn - zf);            // Z 映射
    m[11] = -1.0f;                     // 透视除法（w = -z）
    m[14] = (zn * zf) / (zn - zf);     // Z 偏移
}

// 平移矩阵
static void MatTranslate(float* m, float x, float y, float z)
{
    MatIdentity(m);
    m[12] = x; m[13] = y; m[14] = z;
}

// ============ Win32 窗口回调 ============
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    // 启用 DPI 感知，避免 Windows 缩放导致渲染模糊
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const char* className = "Scene3DClass";
    BackendType selectedBackend = BackendType::DirectX12;

    // ============ 创建 Win32 窗口 ============
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
    HWND hWnd = CreateWindowEx(0, className, "Cacao - 3D Scene",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    std::cout << "=== Cacao 3D Scene ===" << std::endl;

    // ============ 初始化 Cacao RHI ============
    // Instance 是图形 API 的入口，管理后端选择和验证层
    InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = selectedBackend;
    instanceCreateInfo.applicationName = "Scene3D";
    instanceCreateInfo.appVersion = 1;
    instanceCreateInfo.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    Ref<Instance> instance = Instance::Create(instanceCreateInfo);

    // 枚举 GPU 适配器，选择第一个（通常是独立显卡）
    Ref<Adapter> adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    // 创建窗口表面，用于将渲染结果呈现到屏幕
    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<Surface> surface = instance->CreateSurface(nativeWindowHandle);

    // 创建逻辑设备，请求一个图形队列
    DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<Device> device = adapter->CreateDevice(deviceCreateInfo);

    // ============ 创建交换链 ============
    // 交换链管理多个后缓冲区，实现双/三缓冲呈现
    auto surfaceCaps = surface->GetCapabilities(adapter);
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (imageCount > surfaceCaps.maxImageCount && surfaceCaps.maxImageCount != 0)
        imageCount = surfaceCaps.maxImageCount;

    Ref<Swapchain> swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(surfaceCaps.currentExtent)
        .SetFormat(Format::BGRA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo) // 垂直同步
        .SetMinImageCount(imageCount)
        .SetSurface(surface)
        .SetPreTransform(surfaceCaps.currentTransform)
        .SetClipped(true)
        .Build());

    // 帧同步：framesInFlight 决定 CPU 可以提前准备几帧
    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    Ref<Synchronization> synchronization = device->CreateSynchronization(framesInFlight);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    // ============ 立方体几何数据 ============
    // 24 个顶点（每面 4 个，共 6 面），每面独立颜色
    // 使用索引缓冲区减少重复顶点，36 个索引 = 12 个三角形
    Vertex cubeVerts[] = {
        // 前面 (z = -0.5) - 红色
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 0.2f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 0.2f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.2f, 0.2f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.2f, 0.2f}},
        // 后面 (z = 0.5) - 绿色
        {{-0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 0.2f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 0.2f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.2f, 1.0f, 0.2f}},
        {{-0.5f,  0.5f,  0.5f}, {0.2f, 1.0f, 0.2f}},
        // 顶面 (y = 0.5) - 蓝色
        {{-0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.2f, 0.2f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.2f, 0.2f, 1.0f}},
        // 底面 (y = -0.5) - 黄色
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.2f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.2f}},
        // 右面 (x = 0.5) - 青色
        {{ 0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.2f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 1.0f}},
        // 左面 (x = -0.5) - 品红色
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.2f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 1.0f}},
    };

    // 索引数组：每面 2 个三角形，共 36 个索引
    uint32_t cubeIndices[] = {
        0,1,2,  0,2,3,      // 前面
        4,6,5,  4,7,6,      // 后面
        8,9,10, 8,10,11,    // 顶面
        12,14,13, 12,15,14, // 底面
        16,17,18, 16,18,19, // 右面
        20,22,21, 20,23,22, // 左面
    };

    // ============ 创建 GPU 缓冲区 ============
    // 顶点缓冲区：存储立方体的顶点数据
    auto vb = device->CreateBuffer(BufferBuilder()
        .SetSize(sizeof(cubeVerts))
        .SetUsage(BufferUsageFlags::VertexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu) // CPU 可写，GPU 可读
        .SetName("CubeVB")
        .Build());
    memcpy(vb->Map(), cubeVerts, sizeof(cubeVerts));
    vb->Unmap();

    // 索引缓冲区：存储三角形索引，减少顶点重复
    auto ib = device->CreateBuffer(BufferBuilder()
        .SetSize(sizeof(cubeIndices))
        .SetUsage(BufferUsageFlags::IndexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("CubeIB")
        .Build());
    memcpy(ib->Map(), cubeIndices, sizeof(cubeIndices));
    ib->Unmap();

    // Uniform Buffer：每帧更新 MVP 矩阵，256 字节对齐（GPU 要求）
    const uint64_t uniformSize = (sizeof(MVPData) + 255) & ~255;
    std::vector<Ref<Buffer>> uniformBuffers(framesInFlight);
    for (uint32_t i = 0; i < framesInFlight; i++)
    {
        uniformBuffers[i] = device->CreateBuffer(BufferBuilder()
            .SetSize(uniformSize)
            .SetUsage(BufferUsageFlags::UniformBuffer)
            .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
            .SetName("MVPBuffer_" + std::to_string(i))
            .Build());
    }

    // ============ 描述符系统 ============
    // 描述符布局：定义着色器可以访问哪些资源（这里是 1 个 Uniform Buffer）
    auto descriptorSetLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex)
        .Build());

    // 描述符池：分配描述符集的内存池
    auto descriptorPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder()
        .SetMaxSets(framesInFlight)
        .AddPoolSize(DescriptorType::UniformBuffer, framesInFlight)
        .Build());

    // 每帧一个描述符集，绑定对应帧的 Uniform Buffer（避免 CPU/GPU 竞争）
    std::vector<Ref<DescriptorSet>> descriptorSets(framesInFlight);
    for (uint32_t i = 0; i < framesInFlight; i++)
    {
        descriptorSets[i] = descriptorPool->AllocateDescriptorSet(descriptorSetLayout);
        descriptorSets[i]->WriteBuffer({
            0, uniformBuffers[i], 0, sizeof(MVPData), uniformSize, DescriptorType::UniformBuffer, 0
        });
        descriptorSets[i]->Update();
    }

    // ============ 编译 Slang 着色器 ============
    {
        namespace fs = std::filesystem;
        if (fs::exists("shader_cache_scene3d"))
            fs::remove_all("shader_cache_scene3d");
    }
    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_scene3d");

    // 查找着色器文件路径（支持多种相对/绝对路径）
    std::string shaderPath = "scene_3d.slang";
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        auto sep = exeDir.find_last_of("\\/");
        if (sep != std::string::npos) exeDir = exeDir.substr(0, sep + 1);
        std::string candidates[] = {
            exeDir + "scene_3d.slang",
            exeDir + "..\\demos\\scene_3d.slang",
            exeDir + "..\\..\\demos\\scene_3d.slang",
            "demos\\scene_3d.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\scene_3d.slang"
        };
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) { shaderPath = c; break; }
        }
        std::cout << "Shader path: " << shaderPath << std::endl;
    }

    // 编译顶点着色器和片段着色器
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

    // ============ 创建图形管线 ============
    // 管线定义了完整的渲染状态：着色器、顶点格式、光栅化、混合等
    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder()
        .AddSetLayout(descriptorSetLayout)
        .Build());

    auto graphicsPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .SetShaders({vertexShader, fragmentShader})
        .AddVertexBinding(0, sizeof(Vertex))                            // 顶点绑定：步长 = sizeof(Vertex)
        .AddVertexAttribute(0, 0, Format::RGB32_FLOAT, offsetof(Vertex, pos))   // 属性 0: 位置
        .AddVertexAttribute(1, 0, Format::RGB32_FLOAT, offsetof(Vertex, color)) // 属性 1: 颜色
        .SetTopology(PrimitiveTopology::TriangleList)   // 三角形列表
        .SetPolygonMode(PolygonMode::Fill)              // 填充模式
        .SetCullMode(CullMode::None)                    // 不剔除（可看到所有面）
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetLineWidth(1.0f)
        .SetSampleCount(1)                              // 无 MSAA
        .SetDepthTest(false, false)                     // 无深度测试（简单演示）
        .AddColorAttachmentDefault(false)               // 不启用混合
        .AddColorFormat(swapchain->GetFormat())         // 颜色格式与交换链一致
        .SetLayout(pipelineLayout)
        .Build());

    if (!graphicsPipeline)
    {
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
        return 1;
    }
    std::cout << "Pipeline created!" << std::endl;

    // ============ 命令缓冲区 ============
    // 每帧一个命令编码器，记录 GPU 要执行的命令
    std::vector<Ref<CommandBufferEncoder>> commandEncoders(framesInFlight);
    for (auto& e : commandEncoders)
        e = device->CreateCommandBufferEncoder(CommandBufferType::Primary);

    // 跟踪交换链图像是否已初始化（第一次需要 Undefined->ColorAttachment 转换）
    std::vector<bool> swapchainImageInitialized(swapchain->GetImageCount(), false);
    uint32_t frame = 0;
    MSG msg = {};
    bool running = true;
    float angle = 0.0f;

    auto startTime = GetTickCount64();
    std::cout << "3D Scene ready! Rendering..." << std::endl;

    // ============ 主渲染循环 ============
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

        // 基于时间的旋转动画（不受帧率影响）
        float elapsed = (GetTickCount64() - startTime) / 1000.0f;
        angle = elapsed * 0.8f;

        // ---- 构建 MVP 矩阵 ----
        // MVP = Projection * View * Model（列主序）
        // Model: 绕 X 和 Y 轴旋转（实现立方体自转动画）
        // View: 向后平移 3 个单位（模拟相机退后观察）
        // Projection: 透视投影（近大远小效果）
        float model[16], rotY[16], rotX[16], view[16], proj[16], mv[16], mvp[16];
        MatRotateY(rotY, angle);
        MatRotateX(rotX, angle * 0.7f); // X 轴旋转速度稍慢，产生不对称旋转效果
        MatMul(model, rotX, rotY);
        MatTranslate(view, 0, 0, -3.0f);
        float aspect = static_cast<float>(swapchain->GetExtent().width) / swapchain->GetExtent().height;
        MatPerspective(proj, 1.0f, aspect, 0.1f, 100.0f);
        MatMul(mv, view, model);    // ModelView = View * Model
        MatMul(mvp, proj, mv);      // MVP = Proj * ModelView

        // 更新当前帧的 Uniform Buffer
        MVPData mvpData;
        memcpy(mvpData.mvp, mvp, 64);
        memcpy(uniformBuffers[frame]->Map(), &mvpData, sizeof(mvpData));
        uniformBuffers[frame]->Unmap();

        // 等待 GPU 完成此帧的上一次使用
        synchronization->WaitForFrame(frame);

        // 获取下一个可用的交换链图像
        int imageIndex = 0;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
            continue;
        synchronization->ResetFrameFence(frame);

        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        if (backBuffer) backBuffer->CreateDefaultViewIfNeeded();

        // ---- 录制渲染命令 ----
        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true}); // OneTimeSubmit = true

        // 图像状态转换：交换链图像需要从 Present 状态转到可渲染状态
        if (!swapchainImageInitialized[imageIndex])
        {
            cmd->TransitionImage(backBuffer, ImageTransition::UndefinedToColorAttachment);
            swapchainImageInitialized[imageIndex] = true;
        }
        else
        {
            cmd->TransitionImage(backBuffer, ImageTransition::PresentToColorAttachment);
        }

        // 设置渲染目标（动态渲染，无需 RenderPass 对象）
        RenderingAttachmentInfo colorAttachment;
        colorAttachment.Texture = backBuffer;
        colorAttachment.LoadOp = AttachmentLoadOp::Clear;  // 清除为指定颜色
        colorAttachment.StoreOp = AttachmentStoreOp::Store; // 保留渲染结果
        colorAttachment.ClearValue = ClearValue::ColorFloat(0.05f, 0.05f, 0.08f, 1.0f); // 深灰背景

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

        // 绑定管线、描述符集、顶点/索引缓冲区，然后绘制
        cmd->BindGraphicsPipeline(graphicsPipeline);
        std::array<Ref<DescriptorSet>, 1> sets = {descriptorSets[frame]};
        cmd->BindDescriptorSets(graphicsPipeline, 0, sets);
        cmd->BindVertexBuffer(0, vb, 0);
        cmd->BindIndexBuffer(ib, 0, IndexType::UInt32);
        cmd->DrawIndexed(36, 1, 0, 0, 0); // 36 个索引 = 12 个三角形

        cmd->EndRendering();

        // 渲染完成后将图像转换回 Present 状态以供显示
        cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
        cmd->End();

        // 提交命令并呈现
        graphicsQueue->Submit(cmd, synchronization, frame);
        swapchain->Present(presentQueue, synchronization, frame);

        frame = (frame + 1) % framesInFlight;
    }

    // 等待所有 GPU 工作完成后清理
    synchronization->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    std::cout << "3D Scene finished." << std::endl;
    return 0;
}
