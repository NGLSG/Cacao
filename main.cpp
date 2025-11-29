#include <fstream>
#include <vulkan/vulkan.hpp>
#include <windows.h>
#include <iostream>

#include "SynchronizationContext.h"
#include "VkContext.h"

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
    const char* className = "CacaoVulkanWindowClass";
    const char* windowTitle = "Cacao Vulkan Renderer";
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
    GraphicsContextCreateInfo gccInfo{};
    gccInfo.nativeWindowHandle.hInstance = hInstance;
    gccInfo.nativeWindowHandle.hWnd = hWnd;
    gccInfo.enableValidationLayers = true;
    gccInfo.vsyncEnabled = true;
    gccInfo.windowSize.width = width;
    gccInfo.windowSize.height = height;
    std::shared_ptr<VkContext> vkContext = VkContext::Create(gccInfo);
    if (!vkContext)
    {
        std::cerr << "Failed to create Vulkan context." << std::endl;
        return EXIT_FAILURE;
    }
    std::shared_ptr<SynchronizationContext> syncContext = SynchronizationContext::Create(vkContext, 3);
    if (!syncContext)
    {
        std::cerr << "Failed to create synchronization context." << std::endl;
        return EXIT_FAILURE;
    }

    vk::PipelineLayoutCreateInfo plci{};
    vk::PipelineLayout pipelineLayout = vkContext->GetDevice().createPipelineLayout(plci);
    vk::PipelineRenderingCreateInfo pipelineRendering{};
    pipelineRendering.colorAttachmentCount = 1;
    pipelineRendering.pColorAttachmentFormats = &vkContext->GetSurfaceFormat().format;

    // 1. 创建 shader module
    auto vertCode = loadSpv("triangle.vert.spv");
    auto fragCode = loadSpv("triangle.frag.spv");

    vk::ShaderModuleCreateInfo smci{};
    smci.codeSize = vertCode.size() * sizeof(uint32_t);
    smci.pCode = vertCode.data();
    vk::ShaderModule vertModule = vkContext->GetDevice().createShaderModule(smci);

    smci.codeSize = fragCode.size() * sizeof(uint32_t);
    smci.pCode = fragCode.data();
    vk::ShaderModule fragModule = vkContext->GetDevice().createShaderModule(smci);

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vk::PipelineShaderStageCreateInfo{}
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertModule)
        .setPName("main"),
        vk::PipelineShaderStageCreateInfo{}
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragModule)
        .setPName("main"),
    };

    // 2. 其他基本 state（全部可以先用最简单的）
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineRasterizationStateCreateInfo raster{};
    raster.polygonMode = vk::PolygonMode::eFill;
    raster.cullMode = vk::CullModeFlagBits::eBack;
    raster.frontFace = vk::FrontFace::eClockwise;
    raster.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo ms{};
    ms.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo blend{};
    blend.attachmentCount = 1;
    blend.pAttachments = &colorBlendAttachment;

    // 3. 真正的 pipelineCreateInfo
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    pipelineCreateInfo.pDynamicState = &dynamicState;

    pipelineCreateInfo.pNext = &pipelineRendering;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInput;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pRasterizationState = &raster;
    pipelineCreateInfo.pMultisampleState = &ms;
    pipelineCreateInfo.pColorBlendState = &blend;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.pViewportState = &viewportState;

    auto [result, graphicsPipeline] = vkContext->GetDevice().createGraphicsPipeline(nullptr, pipelineCreateInfo);
    if (result != vk::Result::eSuccess)
    {
        std::cerr << "Failed to create graphics pipeline: " << vk::to_string(result) << std::endl;
    }

    vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
                                                .setQueueFamilyIndex(vkContext->GetGraphicsFamilyIndex())
                                                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);;
    vk::CommandPool commandPool = vkContext->GetDevice().createCommandPool(commandPoolInfo);
    vk::CommandBufferAllocateInfo commandBufferAllocInfo = vk::CommandBufferAllocateInfo()
                                                           .setCommandPool(commandPool)
                                                           .setLevel(vk::CommandBufferLevel::ePrimary)
                                                           .setCommandBufferCount(3);
    std::vector<vk::CommandBuffer> commandBuffers =
        vkContext->GetDevice().allocateCommandBuffers(commandBufferAllocInfo);
    auto graphicsQueue = vkContext->GetGraphicsQueue();
    auto presentQueue = vkContext->GetPresentQueue();
    int frame = 0;
    while (isRunning)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) isRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        syncContext->WaitForFrame(frame);

        uint32_t imageIndex;
        try
        {
            imageIndex = syncContext->AcquireNextImageIndex(frame);
        }
        catch (vk::OutOfDateKHRError)
        {
            continue;
        }

        syncContext->ResetFrameFence(frame);

        vk::CommandBuffer cmd = commandBuffers[frame];
        cmd.reset();
        cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        vk::ImageMemoryBarrier barrier{};
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.image = vkContext->GetSwapChainImage(imageIndex);
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            {}, 0, nullptr, 0, nullptr, 1, &barrier
        );

        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.imageView = vkContext->GetSwapChainImageView(imageIndex);
        colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f}));

        vk::RenderingInfo renderInfo{};
        renderInfo.renderArea = vk::Rect2D({0, 0}, vkContext->GetSwapChainExtent());
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachment;

        cmd.beginRendering(renderInfo);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        cmd.setViewport(0, vk::Viewport(0, 0, (float)vkContext->GetSwapChainExtent().width,
                                        (float)vkContext->GetSwapChainExtent().height, 0, 1));
        cmd.setScissor(0, vk::Rect2D({0, 0}, vkContext->GetSwapChainExtent()));

        cmd.draw(3, 1, 0, 0);
        cmd.endRendering();

        vk::ImageMemoryBarrier presentBarrier{};
        presentBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        presentBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        presentBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        presentBarrier.dstAccessMask = {};
        presentBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        presentBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        presentBarrier.image = vkContext->GetSwapChainImage(imageIndex);
        presentBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.layerCount = 1;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {}, 0, nullptr, 0, nullptr, 1, &presentBarrier
        );

        cmd.end();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submit{};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &syncContext->GetImageSemaphore(frame);
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &syncContext->GetRenderSemaphore(frame);

        graphicsQueue.submit(submit, syncContext->GetInFlightFence(frame));

        vk::PresentInfoKHR present{};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &syncContext->GetRenderSemaphore(frame);
        present.swapchainCount = 1;
        present.pSwapchains = &vkContext->GetSwapChain();
        present.pImageIndices = &imageIndex;

        try
        {
            presentQueue.presentKHR(present);
        }
        catch (vk::OutOfDateKHRError)
        {
            // 忽略，下一次循环处理
        }

        // 8. 增加帧计数，实现双重缓冲循环
        frame = (frame + 1) % 3;
    }

    // 退出循环后，等待设备空闲再销毁资源
    vkContext->WaitIdle();

    DestroyWindow(hWnd);
    UnregisterClass(className, hInstance);

    return EXIT_SUCCESS;
}
