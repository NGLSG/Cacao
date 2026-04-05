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

static float g_yaw = 0.0f;
static float g_pitch = 0.0f;
static float g_distance = 10.8f;
static float g_centerX = 2.78f, g_centerY = 2.73f, g_centerZ = 2.795f;
static bool g_dragging = false;
static int g_lastMouseX = 0, g_lastMouseY = 0;
static bool g_cameraChanged = true;

struct Vertex { float pos[3]; float normal[3]; };

static void BuildCornellBoxGeometry(
    std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
    std::vector<float>& vertColors)
{
    auto addQuad = [&](
        float ax, float ay, float az, float bx, float by, float bz,
        float cx, float cy, float cz, float dx, float dy, float dz,
        float nx, float ny, float nz, float r, float g, float b)
    {
        uint32_t base = static_cast<uint32_t>(vertices.size());
        vertices.push_back({{ax,ay,az},{nx,ny,nz}});
        vertices.push_back({{bx,by,bz},{nx,ny,nz}});
        vertices.push_back({{cx,cy,cz},{nx,ny,nz}});
        vertices.push_back({{dx,dy,dz},{nx,ny,nz}});
        indices.insert(indices.end(), {base,base+1,base+2, base,base+2,base+3});
        for (int i = 0; i < 4; i++) { vertColors.push_back(r); vertColors.push_back(g); vertColors.push_back(b); }
    };

    auto addQuadAutoNormal = [&](
        const float v[8][3], int a, int b, int c, int d,
        float r, float g, float bl)
    {
        float e1x = v[b][0]-v[a][0], e1y = v[b][1]-v[a][1], e1z = v[b][2]-v[a][2];
        float e2x = v[d][0]-v[a][0], e2y = v[d][1]-v[a][1], e2z = v[d][2]-v[a][2];
        float nx = e1y*e2z - e1z*e2y;
        float ny = e1z*e2x - e1x*e2z;
        float nz = e1x*e2y - e1y*e2x;
        float nl = sqrtf(nx*nx + ny*ny + nz*nz);
        if (nl > 0) { nx /= nl; ny /= nl; nz /= nl; }
        addQuad(v[a][0],v[a][1],v[a][2], v[b][0],v[b][1],v[b][2],
                v[c][0],v[c][1],v[c][2], v[d][0],v[d][1],v[d][2], nx,ny,nz, r,g,bl);
    };

    // Room walls (original Cornell Box coordinates scaled by 0.01)
    addQuad(0,0,0, 5.528f,0,0, 5.496f,0,5.592f, 0,0,5.592f, 0,1,0, 0.725f,0.71f,0.68f);           // Floor (white)
    addQuad(0,5.488f,0, 0,5.488f,5.592f, 5.56f,5.488f,5.592f, 5.56f,5.488f,0, 0,-1,0, 0.725f,0.71f,0.68f); // Ceiling (white)
    addQuad(0,0,5.592f, 5.56f,0,5.592f, 5.56f,5.488f,5.592f, 0,5.488f,5.592f, 0,0,-1, 0.725f,0.71f,0.68f); // Back wall (white)
    addQuad(0,0,0, 0,0,5.592f, 0,5.488f,5.592f, 0,5.488f,0, 1,0,0, 0.63f,0.065f,0.05f);             // Left wall (red)
    addQuad(5.528f,0,0, 5.56f,5.488f,0, 5.56f,5.488f,5.592f, 5.496f,0,5.592f, -1,0,0, 0.14f,0.45f,0.091f); // Right wall (green)

    // Tall box: standard Cornell Box coordinates (left-rear, rotated)
    float tb[8][3] = {
        {4.23f,3.30f,2.47f}, {2.65f,3.30f,2.96f}, {3.14f,3.30f,4.56f}, {4.72f,3.30f,4.06f},
        {4.23f,0,2.47f},     {2.65f,0,2.96f},     {3.14f,0,4.56f},     {4.72f,0,4.06f}
    };
    addQuadAutoNormal(tb, 0,1,2,3, 0.725f,0.71f,0.68f); // Top
    addQuadAutoNormal(tb, 4,0,3,7, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(tb, 7,3,2,6, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(tb, 6,2,1,5, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(tb, 5,1,0,4, 0.725f,0.71f,0.68f); // Side

    // Short box: standard Cornell Box coordinates (right-front, rotated)
    float sb[8][3] = {
        {1.30f,1.65f,0.65f}, {0.82f,1.65f,2.25f}, {2.40f,1.65f,2.72f}, {2.90f,1.65f,1.14f},
        {1.30f,0,0.65f},     {0.82f,0,2.25f},     {2.40f,0,2.72f},     {2.90f,0,1.14f}
    };
    addQuadAutoNormal(sb, 0,1,2,3, 0.725f,0.71f,0.68f); // Top
    addQuadAutoNormal(sb, 4,0,3,7, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(sb, 7,3,2,6, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(sb, 6,2,1,5, 0.725f,0.71f,0.68f); // Side
    addQuadAutoNormal(sb, 5,1,0,4, 0.725f,0.71f,0.68f); // Side

    // Ceiling area light (must be LAST geometry for IsLightTriangle check)
    addQuad(3.43f,5.486f,2.27f, 3.43f,5.486f,3.32f, 2.13f,5.486f,3.32f, 2.13f,5.486f,2.27f, 0,-1,0, 1.0f,1.0f,1.0f);
}

struct CameraConstants
{
    float invView[16];
    float invProj[16];
    uint32_t width;
    uint32_t height;
    uint32_t frameIndex;
    uint32_t _pad;
};

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
    out[0] = sx; out[1] = ux; out[2] = -fx;
    out[4] = sy; out[5] = uy; out[6] = -fy;
    out[8] = sz; out[9] = uz; out[10] = -fz;
    out[12] = -(sx*ex + sy*ey + sz*ez);
    out[13] = -(ux*ex + uy*ey + uz*ez);
    out[14] = (fx*ex + fy*ey + fz*ez);
    out[15] = 1;
}

static void MakePerspective(float* out, float fovY, float aspect, float zn, float zf)
{
    float tanHalf = tanf(fovY * 0.5f);
    memset(out, 0, 64);
    out[0] = 1.0f / (aspect * tanHalf);
    out[5] = 1.0f / tanHalf;
    out[10] = -(zf + zn) / (zf - zn);
    out[11] = -1.0f;
    out[14] = -2.0f * zf * zn / (zf - zn);
}

static void InvertMatrix4(const float* m, float* out)
{
    float inv[16], det;
    inv[0] = m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
    inv[4] = -m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
    inv[8] = m[4]*m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6]*m[9];
    inv[1] = -m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
    inv[5] = m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
    inv[9] = -m[0]*m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3]*m[9];
    inv[13] = m[0]*m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2]*m[9];
    inv[2] = m[1]*m[6]*m[15]-m[1]*m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2]*m[7]-m[13]*m[3]*m[6];
    inv[6] = -m[0]*m[6]*m[15]+m[0]*m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2]*m[7]+m[12]*m[3]*m[6];
    inv[10] = m[0]*m[5]*m[15]-m[0]*m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1]*m[7]-m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]+m[0]*m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1]*m[6]+m[12]*m[2]*m[5];
    inv[3] = -m[1]*m[6]*m[11]+m[1]*m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10]-m[9]*m[2]*m[7]+m[9]*m[3]*m[6];
    inv[7] = m[0]*m[6]*m[11]-m[0]*m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10]+m[8]*m[2]*m[7]-m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]+m[0]*m[7]*m[9]+m[4]*m[1]*m[11]-m[4]*m[3]*m[9]-m[8]*m[1]*m[7]+m[8]*m[3]*m[5];
    inv[15] = m[0]*m[5]*m[10]-m[0]*m[6]*m[9]-m[4]*m[1]*m[10]+m[4]*m[2]*m[9]+m[8]*m[1]*m[6]-m[8]*m[2]*m[5];
    det = m[0]*inv[0]+m[1]*inv[4]+m[2]*inv[8]+m[3]*inv[12];
    if (fabsf(det) < 1e-12f) { memcpy(out, m, 64); return; }
    det = 1.0f / det;
    for (int i = 0; i < 16; i++) out[i] = inv[i] * det;
}

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
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        float dx = (float)(mx - g_lastMouseX) * 0.005f;
        float dy = (float)(my - g_lastMouseY) * 0.005f;
        g_yaw += dx;
        g_pitch += dy;
        if (g_pitch > 1.5f) g_pitch = 1.5f;
        if (g_pitch < -1.5f) g_pitch = -1.5f;
        g_lastMouseX = mx;
        g_lastMouseY = my;
        g_cameraChanged = true;
        return 0;
    }
    if (uMsg == WM_MOUSEWHEEL) {
        float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
        g_distance -= delta * 0.5f;
        if (g_distance < 1.0f) g_distance = 1.0f;
        if (g_distance > 30.0f) g_distance = 30.0f;
        g_cameraChanged = true;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    BOOL dpiResult = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    std::cout << "DPI awareness result: " << (dpiResult ? "OK" : "FAILED") << std::endl;

    const int width = 1024;
    const int height = 768;
    #ifdef CACAO_USE_VULKAN
    BackendType selectedBackend = BackendType::Vulkan;
    #else
    BackendType selectedBackend = BackendType::DirectX12;
    #endif
    selectedBackend = BackendType::DirectX12;
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "CornellBoxClass";
    RegisterClassEx(&wc);

    RECT windowRect = {0, 0, width, height};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(0, "CornellBoxClass", "Cacao - Cornell Box RT",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    UINT windowDpi = GetDpiForWindow(hWnd);
    std::cout << "Window DPI: " << windowDpi << std::endl;

    RECT gcr;
    GetClientRect(hWnd, &gcr);
    std::cout << "GetClientRect: " << (gcr.right - gcr.left) << "x" << (gcr.bottom - gcr.top) << std::endl;
    if (windowDpi > 96) {
        float dpiScale = windowDpi / 96.0f;
        std::cout << "DPI scale: " << dpiScale << "x" << std::endl;
        std::cout << "If logical, physical would be: " << (int)((gcr.right - gcr.left) * dpiScale) << "x" << (int)((gcr.bottom - gcr.top) * dpiScale) << std::endl;
    }

    std::cout << "=== Cacao Cornell Box Ray Tracing Demo ===" << std::endl;

    InstanceCreateInfo instanceCI;
    instanceCI.type = selectedBackend;
    instanceCI.applicationName = "CornellBoxRT";
    instanceCI.appVersion = 1;
    instanceCI.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    auto instance = Instance::Create(instanceCI);

    auto adapter = instance->EnumerateAdapters().front();
    std::cout << "GPU: " << adapter->GetProperties().name << std::endl;

    NativeWindowHandle nwh;
    nwh.hInst = hInstance;
    nwh.hWnd = hWnd;
    auto surface = instance->CreateSurface(nwh);

    DeviceCreateInfo deviceCI;
    deviceCI.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    deviceCI.CompatibleSurface = surface;
    deviceCI.EnabledFeatures = {DeviceFeature::RayTracingPipeline, DeviceFeature::AccelerationStructure};
    auto device = adapter->CreateDevice(deviceCI);

    // Use physical client rect to avoid DPI scaling mismatch
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Extent2D physicalExtent;
    physicalExtent.width = static_cast<uint32_t>(clientRect.right - clientRect.left);
    physicalExtent.height = static_cast<uint32_t>(clientRect.bottom - clientRect.top);
    std::cout << "Physical client size: " << physicalExtent.width << "x" << physicalExtent.height << std::endl;

    auto surfaceCaps = surface->GetCapabilities(adapter);
    auto swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(physicalExtent)
        .SetFormat(Format::RGBA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo)
        .SetMinImageCount(surfaceCaps.minImageCount + 1)
        .SetSurface(surface)
        .SetPreTransform(surfaceCaps.currentTransform)
        .SetClipped(true)
        .SetUsage(SwapchainUsageFlags::ColorAttachment | SwapchainUsageFlags::TransferDst)
        .Build());

    uint32_t framesInFlight = std::max(1u, swapchain->GetImageCount() - 1);
    auto sync = device->CreateSynchronization(framesInFlight);
    auto graphicsQueue = device->GetQueue(QueueType::Graphics, 0);
    auto presentQueue = surface->GetPresentQueue(device);

    // === Build geometry ===
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<float> colors;
    BuildCornellBoxGeometry(vertices, indices, colors);
    std::cout << "Geometry: " << vertices.size() << " verts, " << indices.size()/3 << " tris" << std::endl;

    auto vb = device->CreateBuffer(BufferBuilder()
        .SetSize(vertices.size() * sizeof(Vertex))
        .SetUsage(BufferUsageFlags::StorageBuffer | BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructure)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(vb->Map(), vertices.data(), vertices.size() * sizeof(Vertex)); vb->Unmap();

    auto ib = device->CreateBuffer(BufferBuilder()
        .SetSize(indices.size() * sizeof(uint32_t))
        .SetUsage(BufferUsageFlags::StorageBuffer | BufferUsageFlags::ShaderDeviceAddress | BufferUsageFlags::AccelerationStructure)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(ib->Map(), indices.data(), indices.size() * sizeof(uint32_t)); ib->Unmap();

    auto cb = device->CreateBuffer(BufferBuilder()
        .SetSize(colors.size() * sizeof(float))
        .SetUsage(BufferUsageFlags::StorageBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(cb->Map(), colors.data(), colors.size() * sizeof(float)); cb->Unmap();

    // === Build acceleration structures ===
    AccelerationStructureCreateInfo blasCI = {};
    blasCI.Type = AccelerationStructureType::BottomLevel;
    AccelerationStructureGeometryDesc geomDesc = {};
    geomDesc.VertexBuffer = vb;
    geomDesc.VertexStride = sizeof(Vertex);
    geomDesc.VertexCount = static_cast<uint32_t>(vertices.size());
    geomDesc.VertexFormat = Format::RGB32_FLOAT;
    geomDesc.IndexBuffer = ib;
    geomDesc.IndexCount = static_cast<uint32_t>(indices.size());
    geomDesc.Opaque = true;
    blasCI.Geometries.push_back(geomDesc);
    auto blas = device->CreateAccelerationStructure(blasCI);

    AccelerationStructureCreateInfo tlasCI = {};
    tlasCI.Type = AccelerationStructureType::TopLevel;
    AccelerationStructureInstance inst = {};
    inst.Transform[0][0] = 1; inst.Transform[1][1] = 1; inst.Transform[2][2] = 1;
    inst.Mask = 0xFF;
    inst.AccelerationStructureAddress = blas->GetDeviceAddress();
    tlasCI.Instances.push_back(inst);
    auto tlas = device->CreateAccelerationStructure(tlasCI);

    auto buildCmd = device->CreateCommandBufferEncoder();
    buildCmd->Begin();
    buildCmd->BuildAccelerationStructure(blas);
    buildCmd->MemoryBarrierFast(MemoryTransition::ComputeWriteToComputeRead);
    buildCmd->BuildAccelerationStructure(tlas);
    buildCmd->End();
    graphicsQueue->Submit({buildCmd});
    graphicsQueue->WaitIdle();
    std::cout << "Acceleration structures built!" << std::endl;

    // === Compile RT shaders (clear stale cache first) ===
    {
        namespace fs = std::filesystem;
        if (fs::exists("shader_cache_cornell"))
            fs::remove_all("shader_cache_cornell");
    }
    // === Compile RT shaders ===
    auto compiler = instance->CreateShaderCompiler();
    compiler->SetCacheDirectory("shader_cache_cornell");

    // Resolve shader path relative to executable
    std::string shaderPath = "cornell_box.slang";
    {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string exeDir(exePath);
        auto sep = exeDir.find_last_of("\\/");
        if (sep != std::string::npos) exeDir = exeDir.substr(0, sep + 1);
        // Try: exe dir, then source demos dir
        std::string candidates[] = {
            exeDir + "cornell_box.slang",
            exeDir + "..\\demos\\cornell_box.slang",
            exeDir + "..\\..\\demos\\cornell_box.slang",
            "demos\\cornell_box.slang",
            "E:\\Dev\\C++\\Cacao\\demos\\cornell_box.slang"
        };
        for (auto& c : candidates) {
            if (std::filesystem::exists(c)) { shaderPath = c; break; }
        }
        std::cout << "Shader path: " << shaderPath << std::endl;
    }

    ShaderCreateInfo rayGenCI;
    rayGenCI.Stage = ShaderStage::RayGen;
    rayGenCI.SourcePath = shaderPath;
    rayGenCI.EntryPoint = "RayGen";
    auto rayGenShader = compiler->CompileOrLoad(device, rayGenCI);

    ShaderCreateInfo closestHitCI;
    closestHitCI.Stage = ShaderStage::RayClosestHit;
    closestHitCI.SourcePath = shaderPath;
    closestHitCI.EntryPoint = "ClosestHit";
    auto closestHitShader = compiler->CompileOrLoad(device, closestHitCI);

    ShaderCreateInfo missCI;
    missCI.Stage = ShaderStage::RayMiss;
    missCI.SourcePath = shaderPath;
    missCI.EntryPoint = "Miss";
    auto missShader = compiler->CompileOrLoad(device, missCI);

    if (!rayGenShader || !closestHitShader || !missShader)
    {
        std::cerr << "Failed to compile RT shaders!" << std::endl;
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        return 0;
    }
    std::cout << "RT shaders compiled!" << std::endl;

    // === Create RT pipeline ===
    auto descLayout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::AccelerationStructure, 1, ShaderStage::AllRayTracing)
        .AddBinding(1, DescriptorType::StorageImage, 1, ShaderStage::RayGen)
        .AddBinding(2, DescriptorType::UniformBuffer, 1, ShaderStage::RayGen)
        .AddBinding(3, DescriptorType::StorageBuffer, 1, ShaderStage::RayClosestHit)
        .AddBinding(4, DescriptorType::StorageBuffer, 1, ShaderStage::RayClosestHit)
        .AddBinding(5, DescriptorType::StorageBuffer, 1, ShaderStage::RayClosestHit)
        .Build());

    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder().AddSetLayout(descLayout).Build());

    Ref<RayTracingPipeline> rtPipeline;
    Ref<ShaderBindingTable> sbt;
    try {
        RayTracingPipelineCreateInfo rtPipelineCI;
        rtPipelineCI.Shaders = {rayGenShader, missShader, closestHitShader};
        rtPipelineCI.MaxRecursionDepth = 2;
        rtPipelineCI.Layout = pipelineLayout;
        rtPipeline = device->CreateRayTracingPipeline(rtPipelineCI);
        std::cout << "RT pipeline created!" << std::endl;

        sbt = device->CreateShaderBindingTable(rtPipeline, 1, 1, 1, 0);
        std::cout << "SBT created!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RT pipeline error: " << e.what() << std::endl;
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        return 1;
    }

    // Use swapchain extent for RT output size
    auto swapExtent = swapchain->GetExtent();
    const uint32_t rtWidth = swapExtent.width;
    const uint32_t rtHeight = swapExtent.height;
    std::cout << "Swapchain size: " << rtWidth << "x" << rtHeight << std::endl;

    // === Camera setup ===
    float viewMat[16], projMat[16], invViewMat[16], invProjMat[16];
    MakeLookAt(viewMat, 2.78f, 2.73f, -8.0f, 2.78f, 2.73f, 2.795f);
    MakePerspective(projMat, 0.6911f, static_cast<float>(rtWidth) / rtHeight, 0.1f, 100.0f);
    InvertMatrix4(viewMat, invViewMat);
    InvertMatrix4(projMat, invProjMat);

    const uint64_t cbSize = (sizeof(CameraConstants) + 255) & ~255;
    auto cameraBuffer = device->CreateBuffer(BufferBuilder()
        .SetSize(cbSize)
        .SetUsage(BufferUsageFlags::UniformBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());

    CameraConstants cam;
    memcpy(cam.invView, invViewMat, 64);
    memcpy(cam.invProj, invProjMat, 64);
    cam.width = rtWidth;
    cam.height = rtHeight;
    cam.frameIndex = 0;
    memcpy(cameraBuffer->Map(), &cam, sizeof(cam));
    cameraBuffer->Unmap();

    // === Create output UAV texture ===
    auto outputImage = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(rtWidth, rtHeight)
        .SetFormat(Format::RGBA8_UNORM)
        .SetUsage(TextureUsageFlags::Storage | TextureUsageFlags::TransferSrc | TextureUsageFlags::Sampled)
        .SetName("RTOutput")
        .Build());
    outputImage->CreateDefaultViewIfNeeded();
    std::cout << "Output image: " << rtWidth << "x" << rtHeight << std::endl;

    // Initialize output texture to UAV state
    {
        auto initCmd = device->CreateCommandBufferEncoder();
        initCmd->Begin();
        TextureBarrier initBarrier;
        initBarrier.Texture = outputImage;
        initBarrier.OldState = ResourceState::Undefined;
        initBarrier.NewState = ResourceState::UnorderedAccess;
        initCmd->PipelineBarrier(SyncScope::AllCommands, SyncScope::AllCommands, std::span<const TextureBarrier>(&initBarrier, 1));
        initCmd->End();
        graphicsQueue->Submit({initCmd});
        graphicsQueue->WaitIdle();
    }

    // === Create descriptor set for RT ===
    auto descPool = device->CreateDescriptorPool(
        DescriptorPoolBuilder().SetMaxSets(1)
        .AddPoolSize(DescriptorType::AccelerationStructure, 1)
        .AddPoolSize(DescriptorType::StorageImage, 1)
        .AddPoolSize(DescriptorType::UniformBuffer, 1)
        .AddPoolSize(DescriptorType::StorageBuffer, 3)
        .Build());

    auto descSet = descPool->AllocateDescriptorSet(descLayout);
    descSet->WriteAccelerationStructure({0, tlas.get(), DescriptorType::AccelerationStructure});
    descSet->WriteTexture({1, outputImage->GetDefaultView(), ResourceState::General, DescriptorType::StorageImage, nullptr, 0});
    descSet->WriteBuffer({2, cameraBuffer, 0, sizeof(CameraConstants), cbSize, DescriptorType::UniformBuffer, 0});
    descSet->WriteBuffer({3, vb, 0, sizeof(Vertex), static_cast<uint64_t>(vertices.size() * sizeof(Vertex)), DescriptorType::StorageBuffer, 0});
    descSet->WriteBuffer({4, ib, 0, sizeof(uint32_t), static_cast<uint64_t>(indices.size() * sizeof(uint32_t)), DescriptorType::StorageBuffer, 0});
    descSet->WriteBuffer({5, cb, 0, sizeof(float), static_cast<uint64_t>(colors.size() * sizeof(float)), DescriptorType::StorageBuffer, 0});
    descSet->Update();

    std::cout << "Cornell Box ray tracing ready! Rendering..." << std::endl;

    std::vector<Ref<CommandBufferEncoder>> cmdEncoders(framesInFlight);
    for (auto& e : cmdEncoders) e = device->CreateCommandBufferEncoder();

    uint32_t frame = 0;
    uint32_t totalFrames = 0;
    MSG msg = {};
    bool running = true;

    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;

        sync->WaitForFrame(frame);

        if (g_cameraChanged) {
            float ex = g_centerX + g_distance * sinf(g_yaw) * cosf(g_pitch);
            float ey = g_centerY + g_distance * sinf(g_pitch);
            float ez = g_centerZ - g_distance * cosf(g_yaw) * cosf(g_pitch);
            MakeLookAt(viewMat, ex, ey, ez, g_centerX, g_centerY, g_centerZ);
            MakePerspective(projMat, 0.6911f, static_cast<float>(rtWidth) / rtHeight, 0.1f, 100.0f);
            InvertMatrix4(viewMat, invViewMat);
            InvertMatrix4(projMat, invProjMat);
            memcpy(cam.invView, invViewMat, 64);
            memcpy(cam.invProj, invProjMat, 64);
            totalFrames = 0;
            g_cameraChanged = false;
        }
        cam.frameIndex = totalFrames;
        memcpy(cameraBuffer->Map(), &cam, sizeof(cam));
        cameraBuffer->Unmap();

        int imageIndex = 0;
        if (swapchain->AcquireNextImage(sync, frame, imageIndex) != Result::Success)
            continue;
        sync->ResetFrameFence(frame);

        auto backBuffer = swapchain->GetBackBuffer(imageIndex);
        if (backBuffer) backBuffer->CreateDefaultViewIfNeeded();

        auto& cmd = cmdEncoders[frame];
        cmd->Reset();
        cmd->Begin({true});

        // 1. Dispatch rays (path tracing with progressive accumulation)
        cmd->BindRayTracingPipeline(rtPipeline);
        cmd->BindRayTracingDescriptorSets(rtPipeline, 0, {&descSet, 1});
        cmd->TraceRays(sbt, rtWidth, rtHeight, 1);

        // 2. Copy RT output to backbuffer
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

        cmd->End();
        graphicsQueue->Submit(cmd, sync, frame);
        swapchain->Present(presentQueue, sync, frame);

        frame = (frame + 1) % framesInFlight;
        totalFrames++;

        if (totalFrames % 100 == 0)
            std::cout << "Accumulated " << totalFrames << " samples" << std::endl;
    }

    graphicsQueue->WaitIdle();
    std::cout << "Cornell Box demo finished." << std::endl;
    DestroyWindow(hWnd);
    return 0;
}
