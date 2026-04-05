// ============================================================
// Shadow Map 示例
//
// 演示基于深度图的阴影映射（Shadow Mapping）技术：
//
// 【原理】
// 1. 第一遍（Shadow Pass）：从光源视角渲染场景，只写入深度值
//    到一张独立的深度纹理（"阴影深度图"）。
// 2. 第二遍（Main Pass）：从相机视角渲染场景，对每个片元：
//    - 将其变换到光源的裁剪空间
//    - 与深度图中存储的最近深度比较
//    - 若当前片元深度 > 深度图值 → 被遮挡 → 在阴影中
//
// 【关键技术】
// - 正交投影（Orthographic）用于方向光的阴影
// - 深度偏移（Depth Bias）防止自阴影条纹
// - PCF（Percentage-Closer Filtering）实现柔和阴影边缘
// - 两遍渲染之间的纹理状态转换（DepthWrite ↔ ShaderRead）
//
// 【场景】地板 + 立方体，方向光从右上方照射
// ============================================================

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include "Cacao.hpp"

using namespace Cacao;

// 相机轨道参数（鼠标拖拽旋转，滚轮缩放）
static float g_yaw = 0.4f, g_pitch = 0.3f, g_dist = 8.0f;
static float g_cx = 0, g_cy = 1.0f, g_cz = 0;
static bool g_dragging = false;
static int g_lastMX = 0, g_lastMY = 0;

// 顶点格式：位置 + 法线 + 颜色（共 36 字节）
struct Vertex { float pos[3]; float normal[3]; float color[3]; };

// 场景参数 UBO，必须与 Slang 着色器中的 SceneParams 布局一致
struct SceneParams {
    float cameraVP[16]; // 相机 VP 矩阵 (Projection × View)
    float lightVP[16];  // 光源 VP 矩阵 (正交投影 × LookAt)
    float lightDir[3];  // 归一化方向光方向
    float _pad0;        // 对齐到 16 字节边界
};

// --- 矩阵工具函数（列主序存储） ---

static void MakeLookAt(float* out, float ex, float ey, float ez, float cx, float cy, float cz)
{
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= fl; fy /= fl; fz /= fl;
    float ux = 0, uy = 1, uz = 0;
    float sx = fy*uz - fz*uy, sy = fz*ux - fx*uz, sz = fx*uy - fy*ux;
    float sl = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= sl; sy /= sl; sz /= sl;
    ux = sy*fz - sz*fy; uy = sz*fx - sx*fz; uz = sx*fy - sy*fx;
    memset(out, 0, 64);
    out[0] = sx;  out[1] = ux;  out[2] = -fx;
    out[4] = sy;  out[5] = uy;  out[6] = -fy;
    out[8] = sz;  out[9] = uz;  out[10] = -fz;
    out[12] = -(sx*ex + sy*ey + sz*ez);
    out[13] = -(ux*ex + uy*ey + uz*ez);
    out[14] = (fx*ex + fy*ey + fz*ez);
    out[15] = 1;
}

static void MakePerspective(float* out, float fovY, float aspect, float zn, float zf)
{
    float t = tanf(fovY * 0.5f);
    memset(out, 0, 64);
    out[0] = 1.0f / (aspect * t);
    out[5] = 1.0f / t;
    out[10] = -(zf + zn) / (zf - zn);
    out[11] = -1.0f;
    out[14] = -2.0f * zf * zn / (zf - zn);
}

static void MakeOrtho(float* out, float l, float r, float b, float t, float n, float f)
{
    memset(out, 0, 64);
    out[0]  = 2.0f / (r - l);
    out[5]  = 2.0f / (t - b);
    out[10] = -2.0f / (f - n);
    out[12] = -(r + l) / (r - l);
    out[13] = -(t + b) / (t - b);
    out[14] = -(f + n) / (f - n);
    out[15] = 1.0f;
}

static void MatMul4x4(const float* a, const float* b, float* out)
{
    float tmp[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            tmp[c*4+r] = 0;
            for (int k = 0; k < 4; k++)
                tmp[c*4+r] += a[k*4+r] * b[c*4+k];
        }
    memcpy(out, tmp, 64);
}

// --- 场景几何体：地板 + 立方体 ---

static void BuildScene(std::vector<Vertex>& verts, std::vector<uint32_t>& idx)
{
    // 添加一个四边形面（2 个三角形，4 个顶点）
    auto addQuad = [&](const float p[4][3], const float n[3], const float c[3]) {
        uint32_t base = (uint32_t)verts.size();
        for (int i = 0; i < 4; i++)
            verts.push_back({{p[i][0],p[i][1],p[i][2]}, {n[0],n[1],n[2]}, {c[0],c[1],c[2]}});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // 地板（y=0 平面，10×10）
    float fp[4][3] = {{-5,0,-5},{5,0,-5},{5,0,5},{-5,0,5}};
    float fn[] = {0,1,0}, fc[] = {0.85f, 0.83f, 0.78f};
    addQuad(fp, fn, fc);

    // 立方体（中心 (0,1,0)，半径 0.75，橙色）
    float h = 0.75f, cy = 1.0f;
    float cc[] = {0.9f, 0.45f, 0.15f};

    float top[4][3] = {{-h,cy+h,-h},{h,cy+h,-h},{h,cy+h,h},{-h,cy+h,h}};
    float tn[] = {0,1,0};
    addQuad(top, tn, cc);

    float bot[4][3] = {{-h,cy-h,h},{h,cy-h,h},{h,cy-h,-h},{-h,cy-h,-h}};
    float bn[] = {0,-1,0};
    addQuad(bot, bn, cc);

    float front[4][3] = {{-h,cy-h,-h},{h,cy-h,-h},{h,cy+h,-h},{-h,cy+h,-h}};
    float frn[] = {0,0,-1};
    addQuad(front, frn, cc);

    float back[4][3] = {{h,cy-h,h},{-h,cy-h,h},{-h,cy+h,h},{h,cy+h,h}};
    float bkn[] = {0,0,1};
    addQuad(back, bkn, cc);

    float left[4][3] = {{-h,cy-h,h},{-h,cy-h,-h},{-h,cy+h,-h},{-h,cy+h,h}};
    float ln[] = {-1,0,0};
    addQuad(left, ln, cc);

    float right[4][3] = {{h,cy-h,-h},{h,cy-h,h},{h,cy+h,h},{h,cy+h,-h}};
    float rn[] = {1,0,0};
    addQuad(right, rn, cc);
}

// --- Window ---

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) { PostQuitMessage(0); return 0; }
    if (uMsg == WM_LBUTTONDOWN) {
        g_dragging = true;
        g_lastMX = LOWORD(lParam); g_lastMY = HIWORD(lParam);
        SetCapture(hwnd); return 0;
    }
    if (uMsg == WM_LBUTTONUP) { g_dragging = false; ReleaseCapture(); return 0; }
    if (uMsg == WM_MOUSEMOVE && g_dragging) {
        int mx = LOWORD(lParam), my = HIWORD(lParam);
        g_yaw   += (float)(mx - g_lastMX) * 0.005f;
        g_pitch += (float)(my - g_lastMY) * 0.005f;
        g_pitch = std::clamp(g_pitch, -1.5f, 1.5f);
        g_lastMX = mx; g_lastMY = my;
        return 0;
    }
    if (uMsg == WM_MOUSEWHEEL) {
        g_dist -= (float)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f * 0.5f;
        g_dist = std::clamp(g_dist, 2.0f, 30.0f);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const int W = 1024, H = 768;
    const uint32_t SM_SIZE = 2048;

    #ifdef CACAO_USE_VULKAN
    BackendType backend = BackendType::Vulkan;
    #else
    BackendType backend = BackendType::DirectX12;
    #endif

    HINSTANCE hInst = GetModuleHandle(nullptr);
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "ShadowMapClass";
    RegisterClassEx(&wc);

    RECT wr = {0, 0, W, H};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(0, "ShadowMapClass", "Cacao - Shadow Map Demo",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, hInst, nullptr);

    // --- RHI setup ---
    InstanceCreateInfo instCI;
    instCI.type = backend;
    instCI.applicationName = "ShadowMapDemo";
    instCI.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    auto instance = Instance::Create(instCI);

    auto adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    NativeWindowHandle nwh;
    nwh.hInst = hInst;
    nwh.hWnd = hWnd;
    auto surface = instance->CreateSurface(nwh);

    DeviceCreateInfo devCI;
    devCI.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    devCI.CompatibleSurface = surface;
    auto device = adapter->CreateDevice(devCI);

    RECT cr;
    GetClientRect(hWnd, &cr);
    Extent2D extent;
    extent.width = (uint32_t)(cr.right - cr.left);
    extent.height = (uint32_t)(cr.bottom - cr.top);

    auto surfCaps = surface->GetCapabilities(adapter);
    auto swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(extent)
        .SetFormat(Format::RGBA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo)
        .SetMinImageCount(surfCaps.minImageCount + 1)
        .SetSurface(surface)
        .SetPreTransform(surfCaps.currentTransform)
        .SetClipped(true)
        .SetUsage(SwapchainUsageFlags::ColorAttachment)
        .Build());

    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    auto sync = device->CreateSynchronization(framesInFlight);
    auto gfxQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    auto swapExt = swapchain->GetExtent();
    const uint32_t rtW = swapExt.width, rtH = swapExt.height;
    std::cout << "Swapchain: " << rtW << "x" << rtH << std::endl;

    // --- Geometry ---
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    BuildScene(vertices, indices);
    uint32_t indexCount = (uint32_t)indices.size();

    auto vb = device->CreateBuffer(BufferBuilder()
        .SetSize(vertices.size() * sizeof(Vertex))
        .SetUsage(BufferUsageFlags::VertexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(vb->Map(), vertices.data(), vertices.size() * sizeof(Vertex)); vb->Unmap();

    auto ib = device->CreateBuffer(BufferBuilder()
        .SetSize(indices.size() * sizeof(uint32_t))
        .SetUsage(BufferUsageFlags::IndexBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(ib->Map(), indices.data(), indices.size() * sizeof(uint32_t)); ib->Unmap();

    // --- 创建阴影深度图和主通道深度缓冲 ---
    // 阴影深度图：2048×2048 D32F 格式，同时用作深度附件（写入）和采样纹理（读取）
    auto shadowMapTex = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(SM_SIZE, SM_SIZE)
        .SetFormat(Format::D32F)
        .SetUsage(TextureUsageFlags::DepthStencilAttachment | TextureUsageFlags::Sampled)
        .SetName("ShadowMap").Build());

    // 创建深度纹理视图（Depth aspect），用于在片段着色器中采样深度值
    TextureViewDesc smViewDesc;
    smViewDesc.Aspect = AspectMask::Depth;
    auto shadowMapView = shadowMapTex->CreateView(smViewDesc);

    // 主通道深度缓冲（仅深度测试用，不需要采样）
    auto mainDepthTex = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(rtW, rtH)
        .SetFormat(Format::D32F)
        .SetUsage(TextureUsageFlags::DepthStencilAttachment)
        .SetName("MainDepth").Build());

    // 阴影图采样器：最近邻滤波，边界返回白色（1.0 = 最远 = 不遮挡）
    auto smSampler = device->CreateSampler(
        SamplerBuilder()
        .SetFilter(Filter::Nearest, Filter::Nearest)
        .SetAddressMode(SamplerAddressMode::ClampToBorder)
        .SetBorderColor(BorderColor::FloatOpaqueWhite)
        .SetAnisotropy(false)
        .Build());

    // --- Shaders ---
    namespace fs = std::filesystem;
    if (fs::exists("shader_cache_shadow")) fs::remove_all("shader_cache_shadow");
    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_shadow");

    std::string shaderPath = "shadow_map.slang";
    {
        char ep[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, ep, MAX_PATH);
        std::string ed(ep);
        auto sep = ed.find_last_of("\\/");
        if (sep != std::string::npos) ed = ed.substr(0, sep + 1);
        std::string candidates[] = {
            ed + "shadow_map.slang",
            ed + "..\\demos\\shadow_map.slang",
            "demos\\shadow_map.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\shadow_map.slang"
        };
        for (auto& c : candidates)
            if (fs::exists(c)) { shaderPath = c; break; }
        std::cout << "Shader: " << shaderPath << std::endl;
    }

    ShaderCreateInfo shadowVsCI; shadowVsCI.Stage = ShaderStage::Vertex;
    shadowVsCI.SourcePath = shaderPath; shadowVsCI.EntryPoint = "ShadowVS";
    auto shadowVS = compiler->CompileOrLoad(device, shadowVsCI);

    ShaderCreateInfo mainVsCI; mainVsCI.Stage = ShaderStage::Vertex;
    mainVsCI.SourcePath = shaderPath; mainVsCI.EntryPoint = "MainVS";
    auto mainVS = compiler->CompileOrLoad(device, mainVsCI);

    ShaderCreateInfo mainPsCI; mainPsCI.Stage = ShaderStage::Fragment;
    mainPsCI.SourcePath = shaderPath; mainPsCI.EntryPoint = "MainPS";
    auto mainPS = compiler->CompileOrLoad(device, mainPsCI);

    if (!shadowVS || !mainVS || !mainPS) {
        std::cerr << "Shader compilation failed!" << std::endl;
        MSG m; while (GetMessage(&m, nullptr, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
        return 1;
    }
    std::cout << "Shaders compiled." << std::endl;

    // --- 描述符布局：两个管线共享同一布局 ---
    // binding 0: UBO（矩阵和光照参数），vertex + fragment 可见
    // binding 1: 阴影深度图（SampledImage），仅 fragment 可见
    // binding 2: 采样器，仅 fragment 可见
    auto descLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex | ShaderStage::Fragment)
        .AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment)
        .AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment)
        .Build());

    auto pipeLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder().AddSetLayout(descLayout).Build());

    // --- 图形管线 ---
    // 阴影管线：仅深度，无颜色输出
    //   - 只需 position 属性（offset=0）
    //   - 无背面剔除（确保所有面都写入深度）
    //   - 启用深度偏移（Depth Bias）减轻自阴影条纹伪影
    auto shadowPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .AddShader(shadowVS)
        .AddVertexBinding(0, sizeof(Vertex))
        .AddVertexAttribute(0, 0, Format::RGB32_FLOAT, 0)
        .SetCullMode(CullMode::None)
        .SetDepthTest(true, true, CompareOp::Less)
        .SetDepthBias(true, 1.25f, 0.0f, 1.75f)
        .SetDepthStencilFormat(Format::D32F)
        .SetLayout(pipeLayout)
        .Build());

    // 主管线：颜色 + 深度，读取 position/normal/color 三个属性
    auto mainPipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .AddShader(mainVS)
        .AddShader(mainPS)
        .AddVertexBinding(0, sizeof(Vertex))
        .AddVertexAttribute(0, 0, Format::RGB32_FLOAT, 0)
        .AddVertexAttribute(1, 0, Format::RGB32_FLOAT, 12)
        .AddVertexAttribute(2, 0, Format::RGB32_FLOAT, 24)
        .SetCullMode(CullMode::Back)
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetDepthTest(true, true, CompareOp::Less)
        .AddColorAttachmentDefault()
        .AddColorFormat(Format::RGBA8_UNORM)
        .SetDepthStencilFormat(Format::D32F)
        .SetLayout(pipeLayout)
        .Build());

    std::cout << "Pipelines created." << std::endl;

    // --- UBO & descriptor set ---
    const uint64_t uboSize = (sizeof(SceneParams) + 255) & ~255;
    auto ubo = device->CreateBuffer(BufferBuilder()
        .SetSize(uboSize)
        .SetUsage(BufferUsageFlags::UniformBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());

    auto descPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder().SetMaxSets(1)
        .AddPoolSize(DescriptorType::UniformBuffer, 1)
        .AddPoolSize(DescriptorType::SampledImage, 1)
        .AddPoolSize(DescriptorType::Sampler, 1)
        .Build());

    auto descSet = descPool->AllocateDescriptorSet(descLayout);
    descSet->WriteBuffer({0, ubo, 0, sizeof(SceneParams), uboSize, DescriptorType::UniformBuffer, 0});
    descSet->WriteTexture({1, shadowMapView, ResourceState::ShaderRead, DescriptorType::SampledImage, nullptr, 0});
    descSet->WriteSampler({2, smSampler, 0});
    descSet->Update();

    // --- 光源设置 ---
    // 方向光：从右上方照射，归一化方向向量
    float lDir[3] = {-1.0f, -2.0f, -1.0f};
    float ll = sqrtf(lDir[0]*lDir[0]+lDir[1]*lDir[1]+lDir[2]*lDir[2]);
    lDir[0] /= ll; lDir[1] /= ll; lDir[2] /= ll;

    // 光源视角矩阵：从 (5,10,5) 看向原点
    // 正交投影：方向光使用正交投影（平行光线，无透视），覆盖场景范围
    float lightView[16], lightProj[16], lightVP[16];
    MakeLookAt(lightView, 5, 10, 5, 0, 0, 0);
    MakeOrtho(lightProj, -8, 8, -8, 8, 1.0f, 25.0f);
    MatMul4x4(lightProj, lightView, lightVP);

    // --- Render loop ---
    std::vector<Ref<CommandBufferEncoder>> cmds(framesInFlight);
    for (auto& c : cmds) c = device->CreateCommandBufferEncoder();
    std::vector<bool> swapImgInit(swapchain->GetImageCount(), false);

    uint32_t frame = 0;
    MSG msg = {};
    bool running = true;

    std::cout << "Rendering... Drag to rotate, scroll to zoom, Esc to quit." << std::endl;

    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        if (!running) break;

        sync->WaitForFrame(frame);

        // Camera
        float ex = g_cx + g_dist * sinf(g_yaw) * cosf(g_pitch);
        float ey = g_cy + g_dist * sinf(g_pitch);
        float ez = g_cz - g_dist * cosf(g_yaw) * cosf(g_pitch);
        float camView[16], camProj[16], camVP[16];
        MakeLookAt(camView, ex, ey, ez, g_cx, g_cy, g_cz);
        MakePerspective(camProj, 0.8f, (float)rtW / rtH, 0.1f, 100.0f);
        MatMul4x4(camProj, camView, camVP);

        SceneParams params;
        memcpy(params.cameraVP, camVP, 64);
        memcpy(params.lightVP, lightVP, 64);
        memcpy(params.lightDir, lDir, 12);
        params._pad0 = 0;
        memcpy(ubo->Map(), &params, sizeof(params)); ubo->Unmap();

        int imageIndex = 0;
        if (swapchain->AcquireNextImage(sync, frame, imageIndex) != Result::Success)
            continue;
        sync->ResetFrameFence(frame);

        auto backBuf = swapchain->GetBackBuffer(imageIndex);
        if (backBuf) backBuf->CreateDefaultViewIfNeeded();

        auto& cmd = cmds[frame];
        cmd->Reset();
        cmd->Begin({true});

        // === 第一遍：阴影深度通道 ===
        // 将阴影深度图从"未定义"转换为"深度写入"状态
        cmd->TransitionImage(shadowMapTex, ImageTransition::UndefinedToDepthAttachment,
            {0, 1, 0, 1, ImageAspectFlags::Depth});
        {
            RenderingInfo ri;
            ri.RenderArea = {0, 0, SM_SIZE, SM_SIZE};
            auto da = std::make_shared<RenderingAttachmentInfo>();
            da->Texture = shadowMapTex;
            da->LoadOp = AttachmentLoadOp::Clear;
            da->StoreOp = AttachmentStoreOp::Store;
            da->ClearDepthStencil = {1.0f, 0};
            ri.DepthAttachment = da;

            cmd->BeginRendering(ri);
            cmd->BindGraphicsPipeline(shadowPipeline);
            cmd->SetViewport({0, 0, (float)SM_SIZE, (float)SM_SIZE, 0, 1});
            cmd->SetScissor({0, 0, SM_SIZE, SM_SIZE});
            cmd->BindVertexBuffer(0, vb, 0);
            cmd->BindIndexBuffer(ib, 0, IndexType::UInt32);
            std::array<Ref<DescriptorSet>, 1> sets = {descSet};
            cmd->BindDescriptorSets(shadowPipeline, 0, sets);
            cmd->DrawIndexed(indexCount, 1, 0, 0, 0);
            cmd->EndRendering();
        }

        // 阴影深度图写入完成 → 转换为"着色器读取"状态，供主通道采样
        cmd->TransitionImage(shadowMapTex, ImageTransition::DepthAttachmentToShaderRead,
            {0, 1, 0, 1, ImageAspectFlags::Depth});

        // === 第二遍：主渲染通道 ===
        if (!swapImgInit[imageIndex]) {
            cmd->TransitionImage(backBuf, ImageTransition::UndefinedToColorAttachment);
            swapImgInit[imageIndex] = true;
        } else {
            cmd->TransitionImage(backBuf, ImageTransition::PresentToColorAttachment);
        }
        cmd->TransitionImage(mainDepthTex, ImageTransition::UndefinedToDepthAttachment,
            {0, 1, 0, 1, ImageAspectFlags::Depth});
        {
            RenderingInfo ri;
            ri.RenderArea = {0, 0, rtW, rtH};
            RenderingAttachmentInfo ca;
            ca.Texture = backBuf;
            ca.LoadOp = AttachmentLoadOp::Clear;
            ca.StoreOp = AttachmentStoreOp::Store;
            ca.ClearValue = ClearValue::ColorFloat(0.05f, 0.05f, 0.1f, 1.0f);
            ri.ColorAttachments.push_back(ca);
            auto da = std::make_shared<RenderingAttachmentInfo>();
            da->Texture = mainDepthTex;
            da->LoadOp = AttachmentLoadOp::Clear;
            da->StoreOp = AttachmentStoreOp::DontCare;
            da->ClearDepthStencil = {1.0f, 0};
            ri.DepthAttachment = da;

            cmd->BeginRendering(ri);
            cmd->BindGraphicsPipeline(mainPipeline);
            cmd->SetViewport({0, 0, (float)rtW, (float)rtH, 0, 1});
            cmd->SetScissor({0, 0, rtW, rtH});
            cmd->BindVertexBuffer(0, vb, 0);
            cmd->BindIndexBuffer(ib, 0, IndexType::UInt32);
            std::array<Ref<DescriptorSet>, 1> sets2 = {descSet};
            cmd->BindDescriptorSets(mainPipeline, 0, sets2);
            cmd->DrawIndexed(indexCount, 1, 0, 0, 0);
            cmd->EndRendering();
        }

        // 渲染完成 → 转换为呈现状态，提交到屏幕
        cmd->TransitionImage(backBuf, ImageTransition::ColorAttachmentToPresent);

        cmd->End();
        gfxQueue->Submit(cmd, sync, frame);
        swapchain->Present(presentQueue, sync, frame);
        frame = (frame + 1) % framesInFlight;
    }

    gfxQueue->WaitIdle();
    std::cout << "Shadow map demo finished." << std::endl;
    DestroyWindow(hWnd);
    return 0;
}
