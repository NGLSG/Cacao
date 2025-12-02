#define NOMINMAX
#include <fstream>
#include <windows.h>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include "Cacao.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace Cacao;

// 线程数量
const uint32_t NUM_THREADS = std::max(1u, std::thread::hardware_concurrency() - 1);

// 精灵实例数据 (与着色器匹配)
struct SpriteInstance
{
    float position[2];  // 精灵位置 (NDC: -1 到 1)
    float scale[2];     // 精灵缩放
    float uvRect[4];    // UV矩形 (u0, v0, u1, v1)
    float color[4];     // 颜色调制
    float rotation;     // 旋转角度 (弧度)
    float _padding[3];  // 对齐到64字节
};

static_assert(sizeof(SpriteInstance) == 64, "SpriteInstance must be 64 bytes");

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

// 加载纹理辅助函数
Ref<Texture> LoadTextureFromFile(const Ref<Device>& device, const Ref<CommandBufferEncoder>& cmd,
                                  const Ref<Queue>& queue, const char* filepath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    if (!pixels)
    {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return nullptr;
    }
    
    uint64_t imageSize = texWidth * texHeight * 4;
    
    // 创建staging buffer
    auto stagingBuffer = device->CreateBuffer(
        BufferBuilder()
            .SetSize(imageSize)
            .SetUsage(BufferUsageFlags::TransferSrc)
            .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
            .SetName("StagingBuffer")
            .Build());
    
    // 复制像素数据到staging buffer
    void* data = stagingBuffer->Map();
    memcpy(data, pixels, imageSize);
    stagingBuffer->Unmap();
    stagingBuffer->Flush();
    
    stbi_image_free(pixels);
    
    // 创建纹理
    auto texture = device->CreateTexture(
        TextureBuilder()
            .SetType(TextureType::Texture2D)
            .SetSize(texWidth, texHeight)
            .SetMipLevels(1)
            .SetArrayLayers(1)
            .SetFormat(Format::RGBA8_UNORM)
            .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
            .SetName(filepath)
            .Build());
    
    // 记录命令: 转换布局 + 复制
    cmd->Reset();
    cmd->Begin({true});
    
    cmd->TransitionImage(texture, ImageTransition::UndefinedToTransferDst);
    
    BufferImageCopy region;
    region.BufferOffset = 0;
    region.BufferRowLength = 0;
    region.BufferImageHeight = 0;
    region.ImageSubresource.AspectMask = ImageAspectFlags::Color;
    region.ImageSubresource.MipLevel = 0;
    region.ImageSubresource.BaseArrayLayer = 0;
    region.ImageSubresource.LayerCount = 1;
    region.ImageOffsetX = 0;
    region.ImageOffsetY = 0;
    region.ImageOffsetZ = 0;
    region.ImageExtentWidth = texWidth;
    region.ImageExtentHeight = texHeight;
    region.ImageExtentDepth = 1;
    
    cmd->CopyBufferToImage(stagingBuffer, texture, ImageLayout::TransferDst, {&region, 1});
    
    cmd->TransitionImage(texture, ImageTransition::TransferDstToShaderRead);
    
    cmd->End();
    
    // 提交并等待完成
    queue->Submit(cmd);
    queue->WaitIdle();
    
    return texture;
}

int main()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const char* className = "CacaoWindowClass";
    const char* windowTitle = "Cacao Sprite Batch Renderer";
    const int width = 1280;
    const int height = 720;

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

    std::cout << "=== Cacao Sprite Batch Renderer ===" << std::endl;

    // === 初始化Cacao ===
    InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.type = BackendType::Vulkan;
    instanceCreateInfo.applicationName = "SpriteBatchDemo";
    instanceCreateInfo.appVersion = 1;
    instanceCreateInfo.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    Ref<Instance> instance = Instance::Create(instanceCreateInfo);
    if (!instance)
    {
        std::cerr << "Fatal Error: Failed to create Cacao instance." << std::endl;
        return EXIT_FAILURE;
    }
    
    Ref<Adapter> adapter = instance->EnumerateAdapters().front();
    auto properties = adapter->GetProperties();
    std::cout << "GPU: " << properties.name << std::endl;

    NativeWindowHandle nativeWindowHandle;
    nativeWindowHandle.hInst = hInstance;
    nativeWindowHandle.hWnd = hWnd;
    Ref<Surface> surface = instance->CreateSurface(nativeWindowHandle);
    
    std::vector<QueueRequest> queueRequests = {{QueueType::Graphics, 1, 1.0f}};
    DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.QueueRequests = queueRequests;
    deviceCreateInfo.CompatibleSurface = surface;
    Ref<Device> device = adapter->CreateDevice(deviceCreateInfo);
    
    auto surfaceCaps = surface->GetCapabilities(adapter);
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (imageCount > surfaceCaps.maxImageCount && surfaceCaps.maxImageCount != 0)
        imageCount = surfaceCaps.maxImageCount;
    
    Ref<Swapchain> swapchain = device->CreateSwapchain(
        SwapchainBuilder()
            .SetExtent(surfaceCaps.currentExtent)
            .SetFormat(Format::BGRA8_UNORM)
            .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
            .SetPresentMode(PresentMode::Mailbox)
            .SetMinImageCount(imageCount)
            .SetSurface(surface)
            .SetPreTransform(surfaceCaps.currentTransform)
            .SetClipped(true)
            .Build());
    
    Ref<Synchronization> synchronization = device->CreateSynchronization(swapchain->GetImageCount());
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);
    
    // === 加载精灵纹理 ===
    auto uploadCmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
    Ref<Texture> spriteTexture = LoadTextureFromFile(device, uploadCmd, graphicsQueue, "sprite.png");
    if (!spriteTexture)
    {
        // 如果没有sprite.png，创建一个简单的白色纹理
        std::cout << "Creating default white texture..." << std::endl;
        spriteTexture = device->CreateTexture(
            TextureBuilder()
                .SetType(TextureType::Texture2D)
                .SetSize(64, 64)
                .SetFormat(Format::RGBA8_UNORM)
                .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
                .SetName("DefaultWhite")
                .Build());
        
        // 创建白色像素数据
        std::vector<uint8_t> whitePixels(64 * 64 * 4, 255);
        
        auto stagingBuffer = device->CreateBuffer(
            BufferBuilder()
                .SetSize(whitePixels.size())
                .SetUsage(BufferUsageFlags::TransferSrc)
                .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
                .Build());
        void* data = stagingBuffer->Map();
        memcpy(data, whitePixels.data(), whitePixels.size());
        stagingBuffer->Unmap();
        stagingBuffer->Flush();
        
        uploadCmd->Reset();
        uploadCmd->Begin({true});
        uploadCmd->TransitionImage(spriteTexture, ImageTransition::UndefinedToTransferDst);
        
        BufferImageCopy region;
        region.ImageSubresource.AspectMask = ImageAspectFlags::Color;
        region.ImageSubresource.LayerCount = 1;
        region.ImageExtentWidth = 64;
        region.ImageExtentHeight = 64;
        region.ImageExtentDepth = 1;
        uploadCmd->CopyBufferToImage(stagingBuffer, spriteTexture, ImageLayout::TransferDst, {&region, 1});
        
        uploadCmd->TransitionImage(spriteTexture, ImageTransition::TransferDstToShaderRead);
        uploadCmd->End();
        graphicsQueue->Submit(uploadCmd);
        graphicsQueue->WaitIdle();
    }
    spriteTexture->CreateDefaultViewIfNeeded();
    
    // === 创建采样器 (Builder模式) ===
    auto sampler = device->CreateSampler(
        SamplerBuilder()
            .SetFilter(Filter::Linear, Filter::Linear)
            .SetAddressMode(SamplerAddressMode::ClampToEdge)
            .SetAnisotropy(false)
            .Build());
    
    // === 精灵实例数据 ===
    constexpr uint32_t SPRITE_COUNT = 1000000;
    
    // 使用紧凑的SOA布局提高缓存效率
    struct SpriteVelocity { float vx, vy, vr; };
    std::vector<SpriteInstance> sprites(SPRITE_COUNT);
    std::vector<SpriteVelocity> velocities(SPRITE_COUNT);
    
    // 随机初始化精灵 (多线程)
    std::cout << "Initializing " << SPRITE_COUNT << " sprites with " << NUM_THREADS << " threads..." << std::endl;
    {
        std::vector<std::thread> initThreads;
        const uint32_t spritesPerThread = SPRITE_COUNT / NUM_THREADS;
        
        for (uint32_t t = 0; t < NUM_THREADS; t++)
        {
            uint32_t start = t * spritesPerThread;
            uint32_t end = (t == NUM_THREADS - 1) ? SPRITE_COUNT : start + spritesPerThread;
            
            initThreads.emplace_back([&sprites, &velocities, start, end, t]() {
                std::mt19937 rng(42 + t);
                std::uniform_real_distribution<float> posDist(-0.9f, 0.9f);
                std::uniform_real_distribution<float> scaleDist(0.01f, 0.03f);
                std::uniform_real_distribution<float> colorDist(0.5f, 1.0f);
                std::uniform_real_distribution<float> rotDist(0.0f, 6.28318f);
                std::uniform_real_distribution<float> velDist(-0.3f, 0.3f);
                
                for (uint32_t i = start; i < end; i++)
                {
                    float scale = scaleDist(rng);
                    sprites[i].position[0] = posDist(rng);
                    sprites[i].position[1] = posDist(rng);
                    sprites[i].scale[0] = scale;
                    sprites[i].scale[1] = scale;
                    sprites[i].uvRect[0] = 0.0f; sprites[i].uvRect[1] = 0.0f;
                    sprites[i].uvRect[2] = 1.0f; sprites[i].uvRect[3] = 1.0f;
                    sprites[i].color[0] = colorDist(rng);
                    sprites[i].color[1] = colorDist(rng);
                    sprites[i].color[2] = colorDist(rng);
                    sprites[i].color[3] = 1.0f;
                    sprites[i].rotation = rotDist(rng);
                    velocities[i].vx = velDist(rng);
                    velocities[i].vy = velDist(rng);
                    velocities[i].vr = velDist(rng) * 2.0f;
                }
            });
        }
        for (auto& t : initThreads) t.join();
    }
    
    // === 创建多缓冲精灵实例缓冲区 (三缓冲避免GPU等待) ===
    const uint32_t BUFFER_COUNT = swapchain->GetImageCount();
    std::vector<Ref<Buffer>> spriteBuffers(BUFFER_COUNT);
    std::vector<void*> mappedBuffers(BUFFER_COUNT);
    const uint64_t spriteBufferSize = sizeof(SpriteInstance) * SPRITE_COUNT;
    
    for (uint32_t i = 0; i < BUFFER_COUNT; i++)
    {
        spriteBuffers[i] = device->CreateBuffer(
            BufferBuilder()
                .SetSize(spriteBufferSize)
                .SetUsage(BufferUsageFlags::StorageBuffer)
                .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
                .SetName("SpriteBuffer_" + std::to_string(i))
                .Build());
        mappedBuffers[i] = spriteBuffers[i]->Map(); // 持久映射
    }
    
    // === 创建描述符集布局 (Builder模式) ===
    auto descriptorSetLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
            .AddBinding(0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex)
            .AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment)
            .AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment)
            .Build());
    
    // === 创建描述符池和描述符集 (每个缓冲区一个) ===
    auto descriptorPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder()
            .SetMaxSets(BUFFER_COUNT)
            .AddPoolSize(DescriptorType::StorageBuffer, BUFFER_COUNT)
            .AddPoolSize(DescriptorType::SampledImage, BUFFER_COUNT)
            .AddPoolSize(DescriptorType::Sampler, BUFFER_COUNT)
            .Build());
    
    std::vector<Ref<DescriptorSet>> descriptorSets(BUFFER_COUNT);
    for (uint32_t i = 0; i < BUFFER_COUNT; i++)
    {
        descriptorSets[i] = descriptorPool->AllocateDescriptorSet(descriptorSetLayout);
        descriptorSets[i]->WriteBuffer({0, spriteBuffers[i], 0, spriteBufferSize, DescriptorType::StorageBuffer, 0});
        descriptorSets[i]->WriteTexture({1, spriteTexture->GetDefaultView(), ImageLayout::ShaderReadOnly, DescriptorType::SampledImage, nullptr, 0});
        descriptorSets[i]->WriteSampler({2, sampler, 0});
        descriptorSets[i]->Update();
    }
    
    // === 编译着色器 ===
    auto compiler = instance->CreateShaderCompiler();
    
    ShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.Stage = ShaderStage::Vertex;
    shaderCreateInfo.SourcePath = "sprite.slang";
    shaderCreateInfo.EntryPoint = "mainVS";
    auto vertexShader = compiler->CompileOrLoad(device, shaderCreateInfo);
    
    shaderCreateInfo.Stage = ShaderStage::Fragment;
    shaderCreateInfo.EntryPoint = "mainPS";
    auto fragmentShader = compiler->CompileOrLoad(device, shaderCreateInfo);
    
    if (!vertexShader || !fragmentShader)
    {
        std::cerr << "Fatal Error: Failed to compile shaders." << std::endl;
        return EXIT_FAILURE;
    }
    
    // === 创建管线 (Builder模式) ===
    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder()
            .AddSetLayout(descriptorSetLayout)
            .Build());
    
    auto graphicsPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
            .SetShaders({vertexShader, fragmentShader})
            .SetTopology(PrimitiveTopology::TriangleList)
            .SetPolygonMode(PolygonMode::Fill)
            .SetCullMode(CullMode::None)
            .SetFrontFace(FrontFace::CounterClockwise)
            .SetLineWidth(1.0f)
            .SetSampleCount(1)
            .SetDepthTest(false, false)
            .AddColorAttachmentDefault(true)  // Alpha混合
            .AddColorFormat(swapchain->GetFormat())
            .SetLayout(pipelineLayout)
            .Build());
    if (!graphicsPipeline)
    {
        std::cerr << "Fatal Error: Failed to create graphics pipeline." << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Rendering " << SPRITE_COUNT << " sprites with " << NUM_THREADS << " update threads..." << std::endl;
    
    // === 命令缓冲区 ===
    std::vector<Ref<CommandBufferEncoder>> commandEncoders(BUFFER_COUNT);
    for (uint32_t i = 0; i < commandEncoders.size(); i++)
        commandEncoders[i] = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
    
    // === 多线程更新系统 ===
    std::atomic<bool> isRunning{true};
    std::atomic<float> globalDeltaTime{0.016f};
    std::atomic<int> updateFrame{0};
    std::atomic<bool> updateReady{false};
    
    // 更新线程函数
    auto updateWorker = [&](uint32_t threadId, uint32_t start, uint32_t end) {
        while (isRunning.load(std::memory_order_relaxed))
        {
            // 等待下一帧
            while (!updateReady.load(std::memory_order_acquire) && isRunning.load(std::memory_order_relaxed))
                std::this_thread::yield();
            
            if (!isRunning.load(std::memory_order_relaxed)) break;
            
            float dt = globalDeltaTime.load(std::memory_order_relaxed);
            int targetFrame = updateFrame.load(std::memory_order_relaxed);
            SpriteInstance* dst = static_cast<SpriteInstance*>(mappedBuffers[targetFrame]);
            
            // 更新并直接写入映射缓冲区
            for (uint32_t i = start; i < end; i++)
            {
                float px = sprites[i].position[0] + velocities[i].vx * dt;
                float py = sprites[i].position[1] + velocities[i].vy * dt;
                
                // 边界反弹
                if (px < -1.0f || px > 1.0f) { velocities[i].vx = -velocities[i].vx; px = std::clamp(px, -1.0f, 1.0f); }
                if (py < -1.0f || py > 1.0f) { velocities[i].vy = -velocities[i].vy; py = std::clamp(py, -1.0f, 1.0f); }
                
                sprites[i].position[0] = px;
                sprites[i].position[1] = py;
                sprites[i].rotation += velocities[i].vr * dt;
                
                // 直接写入GPU缓冲区
                dst[i] = sprites[i];
            }
        }
    };
    
    // 启动更新线程
    std::vector<std::thread> updateThreads;
    const uint32_t spritesPerThread = SPRITE_COUNT / NUM_THREADS;
    for (uint32_t t = 0; t < NUM_THREADS; t++)
    {
        uint32_t start = t * spritesPerThread;
        uint32_t end = (t == NUM_THREADS - 1) ? SPRITE_COUNT : start + spritesPerThread;
        updateThreads.emplace_back(updateWorker, t, start, end);
    }
    
    // 等待更新完成的计数器
    std::atomic<uint32_t> threadsCompleted{0};
    
    // === 主循环 ===
    MSG msg = {};
    int frame = 0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float fpsTimer = 0.0f;
    double totalUpdateTime = 0.0;
    double totalRenderTime = 0.0;
    
    while (isRunning.load())
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // 计算deltaTime
        float deltaTime = std::chrono::duration<float>(frameStart - lastTime).count();
        lastTime = frameStart;
        deltaTime = std::min(deltaTime, 0.1f); // 限制最大dt
        
        // FPS计数
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f)
        {
            double avgUpdate = totalUpdateTime / frameCount * 1000.0;
            double avgRender = totalRenderTime / frameCount * 1000.0;
            std::cout << "FPS: " << frameCount 
                      << " | Sprites: " << SPRITE_COUNT 
                      << " | Update: " << avgUpdate << "ms"
                      << " | Render: " << avgRender << "ms" << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
            totalUpdateTime = 0.0;
            totalRenderTime = 0.0;
        }
        
        // 处理消息
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) isRunning.store(false);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // 等待GPU完成当前帧
        synchronization->WaitForFrame(frame);
        
        auto updateStart = std::chrono::high_resolution_clock::now();
        
        // 触发多线程更新
        globalDeltaTime.store(deltaTime, std::memory_order_relaxed);
        updateFrame.store(frame, std::memory_order_relaxed);
        updateReady.store(true, std::memory_order_release);
        
        // 等待更新完成 (简化版: 主线程也参与更新)
        {
            float dt = deltaTime;
            SpriteInstance* dst = static_cast<SpriteInstance*>(mappedBuffers[frame]);
            // 主线程不参与更新，等待工作线程
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        updateReady.store(false, std::memory_order_release);
        
        // Flush缓冲区
        spriteBuffers[frame]->Flush();
        
        auto updateEnd = std::chrono::high_resolution_clock::now();
        totalUpdateTime += std::chrono::duration<double>(updateEnd - updateStart).count();
        
        // 获取下一张图像
        int imageIndex;
        if (swapchain->AcquireNextImage(synchronization, frame, imageIndex) != Result::Success)
            continue;
        synchronization->ResetFrameFence(frame);
        
        auto renderStart = std::chrono::high_resolution_clock::now();
        
        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        backBuffer->CreateDefaultViewIfNeeded();
        
        auto cmd = commandEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});
        
        cmd->TransitionImage(backBuffer, ImageTransition::UndefinedToColorAttachment);
        
        RenderingAttachmentInfo colorAttachment;
        colorAttachment.Texture = backBuffer;
        colorAttachment.LoadOp = AttachmentLoadOp::Clear;
        colorAttachment.StoreOp = AttachmentStoreOp::Store;
        colorAttachment.ClearValue = ClearValue::ColorFloat(0.05f, 0.05f, 0.08f, 1.0f);
        
        RenderingInfo renderInfo;
        renderInfo.RenderArea = {0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height};
        renderInfo.LayerCount = 1;
        renderInfo.ColorAttachments = {colorAttachment};
        
        cmd->BeginRendering(renderInfo);
        cmd->SetViewport({0.0f, 0.0f,
            static_cast<float>(swapchain->GetExtent().width),
            static_cast<float>(swapchain->GetExtent().height),
            0.0f, 1.0f});
        cmd->SetScissor({0, 0, swapchain->GetExtent().width, swapchain->GetExtent().height});
        
        cmd->BindGraphicsPipeline(graphicsPipeline);
        std::array<Ref<DescriptorSet>, 1> sets = {descriptorSets[frame]};
        cmd->BindDescriptorSets(graphicsPipeline, 0, sets);
        
        // 实例化绘制
        cmd->Draw(6, SPRITE_COUNT, 0, 0);
        
        cmd->EndRendering();
        cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
        cmd->End();
        
        auto renderEnd = std::chrono::high_resolution_clock::now();
        totalRenderTime += std::chrono::duration<double>(renderEnd - renderStart).count();
        
        graphicsQueue->Submit(cmd, synchronization, frame);
        swapchain->Present(presentQueue, synchronization, frame);
        frame = (frame + 1) % BUFFER_COUNT;
    }
    
    // 停止更新线程
    isRunning.store(false);
    updateReady.store(true); // 唤醒线程以便退出
    for (auto& t : updateThreads) t.join();
    
    // 取消映射
    for (uint32_t i = 0; i < BUFFER_COUNT; i++)
        spriteBuffers[i]->Unmap();
    
    device->WaitIdle();
    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);
    
    return EXIT_SUCCESS;
}
