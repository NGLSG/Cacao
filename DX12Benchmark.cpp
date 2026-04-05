#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <random>
#include <thread>
#include <algorithm>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

static constexpr uint32_t WIDTH = 1280;
static constexpr uint32_t HEIGHT = 720;
static constexpr uint32_t SPRITE_COUNT = 1000000;
static constexpr uint32_t BUFFER_COUNT = 2;

struct SpriteInstance {
    float position[2];
    float scale[2];
    float uvRect[4];
    float color[4];
    float rotation;
    float _padding[3];
};
static_assert(sizeof(SpriteInstance) == 64);

static const char* g_shaderSrc = R"(
struct SpriteInstance {
    float2 position;
    float2 scale;
    float4 uvRect;
    float4 color;
    float rotation;
    float3 _pad;
};
StructuredBuffer<SpriteInstance> sprites : register(t0);

struct VSOut { float4 pos : SV_Position; float4 col : COLOR; };

static const float2 quad[6] = {
    float2(-0.5,-0.5), float2(0.5,-0.5), float2(-0.5,0.5),
    float2(-0.5,0.5),  float2(0.5,-0.5), float2(0.5,0.5)
};

VSOut VSMain(uint vid : SV_VertexID, uint iid : SV_InstanceID) {
    SpriteInstance s = sprites[iid];
    float2 v = quad[vid] * s.scale + s.position;
    VSOut o;
    o.pos = float4(v, 0, 1);
    o.col = s.color;
    return o;
}

float4 PSMain(VSOut i) : SV_Target { return i.col; }
)";

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE || (m == WM_KEYDOWN && w == VK_ESCAPE)) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(h, m, w, l);
}

int main() {
    WNDCLASSA wc = {}; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "DX12Bench";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("DX12Bench", "DX12 Raw Benchmark", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, nullptr, nullptr, wc.hInstance, nullptr);

    ComPtr<ID3D12Debug> debug;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    if (debug) debug->EnableDebugLayer();

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    ComPtr<IDXGIAdapter1> adapter;
    factory->EnumAdapters1(0, &adapter);

    ComPtr<ID3D12Device> device;
    D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;
    device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue));

    DXGI_SWAP_CHAIN_DESC1 scd = {};
    scd.Width = WIDTH; scd.Height = HEIGHT;
    scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count = 1; scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = BUFFER_COUNT; scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &scd, nullptr, nullptr, &sc1);
    ComPtr<IDXGISwapChain3> swapchain;
    sc1.As(&swapchain);

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    D3D12_DESCRIPTOR_HEAP_DESC rtvhd = {}; rtvhd.NumDescriptors = BUFFER_COUNT;
    rtvhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&rtvhd, IID_PPV_ARGS(&rtvHeap));
    uint32_t rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    ComPtr<ID3D12Resource> renderTargets[BUFFER_COUNT];
    for (uint32_t i = 0; i < BUFFER_COUNT; i++) {
        swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        h.ptr += i * rtvSize;
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, h);
    }

    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    cmdList->Close();

    ComPtr<ID3D12Fence> fence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    uint64_t fenceValue = 0;

    // Sprite buffer (UPLOAD)
    uint64_t bufSize = SPRITE_COUNT * sizeof(SpriteInstance);
    ComPtr<ID3D12Resource> spriteBuf;
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd = {}; bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = bufSize; bd.Height = 1; bd.DepthOrArraySize = 1; bd.MipLevels = 1;
    bd.Format = DXGI_FORMAT_UNKNOWN; bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&spriteBuf));

    SpriteInstance* mapped = nullptr;
    spriteBuf->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distPos(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distColor(0.2f, 1.0f);
    for (uint32_t i = 0; i < SPRITE_COUNT; i++) {
        mapped[i].position[0] = distPos(rng); mapped[i].position[1] = distPos(rng);
        mapped[i].scale[0] = 0.003f; mapped[i].scale[1] = 0.003f;
        mapped[i].uvRect[0] = 0; mapped[i].uvRect[1] = 0; mapped[i].uvRect[2] = 1; mapped[i].uvRect[3] = 1;
        mapped[i].color[0] = distColor(rng); mapped[i].color[1] = distColor(rng);
        mapped[i].color[2] = distColor(rng); mapped[i].color[3] = 1.0f;
        mapped[i].rotation = 0;
    }

    // SRV heap
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    D3D12_DESCRIPTOR_HEAP_DESC srvhd = {}; srvhd.NumDescriptors = 1;
    srvhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvhd, IID_PPV_ARGS(&srvHeap));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = SPRITE_COUNT;
    srvDesc.Buffer.StructureByteStride = sizeof(SpriteInstance);
    device->CreateShaderResourceView(spriteBuf.Get(), &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

    // Compile shaders
    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    if (FAILED(D3DCompile(g_shaderSrc, strlen(g_shaderSrc), "sprite", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vsBlob, &errBlob))) {
        printf("VS compile error: %s\n", (char*)errBlob->GetBufferPointer()); return 1;
    }
    if (FAILED(D3DCompile(g_shaderSrc, strlen(g_shaderSrc), "sprite", nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &psBlob, &errBlob))) {
        printf("PS compile error: %s\n", (char*)errBlob->GetBufferPointer()); return 1;
    }

    // Root signature: 1 descriptor table with 1 SRV
    D3D12_DESCRIPTOR_RANGE range = {}; range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1; range.BaseShaderRegister = 0;
    D3D12_ROOT_PARAMETER rp = {}; rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rp.DescriptorTable.NumDescriptorRanges = 1; rp.DescriptorTable.pDescriptorRanges = &range;
    rp.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_ROOT_SIGNATURE_DESC rsd = {}; rsd.NumParameters = 1; rsd.pParameters = &rp;
    rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    ComPtr<ID3DBlob> sigBlob;
    D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    ComPtr<ID3D12RootSignature> rootSig;
    device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psd = {};
    psd.pRootSignature = rootSig.Get();
    psd.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psd.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psd.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psd.SampleMask = UINT_MAX;
    psd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psd.NumRenderTargets = 1; psd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psd.SampleDesc.Count = 1;
    ComPtr<ID3D12PipelineState> pso;
    device->CreateGraphicsPipelineState(&psd, IID_PPV_ARGS(&pso));

    printf("=== DX12 Raw Benchmark ===\n");
    printf("Sprites: %u\n", SPRITE_COUNT);

    auto startTime = std::chrono::steady_clock::now();
    uint32_t frameCount = 0;
    double totalUpdate = 0, totalRender = 0, totalPresent = 0;

    MSG msg = {};
    while (true) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            DispatchMessageA(&msg);
        }

        // Update sprites
        auto t0 = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float>(t0 - startTime).count();
        uint32_t cores = std::max(1u, std::thread::hardware_concurrency() - 1);
        uint32_t perThread = SPRITE_COUNT / cores;
        std::vector<std::thread> threads;
        for (uint32_t t = 0; t < cores; t++) {
            uint32_t start = t * perThread;
            uint32_t end = (t == cores - 1) ? SPRITE_COUNT : start + perThread;
            threads.emplace_back([mapped, start, end, time]() {
                for (uint32_t i = start; i < end; i++) {
                    float phase = static_cast<float>(i) * 0.001f;
                    mapped[i].position[0] += sinf(time * 2.0f + phase) * 0.0001f;
                    mapped[i].position[1] += cosf(time * 2.0f + phase) * 0.0001f;
                    if (mapped[i].position[0] > 1.0f) mapped[i].position[0] = -1.0f;
                    if (mapped[i].position[1] > 1.0f) mapped[i].position[1] = -1.0f;
                }
            });
        }
        for (auto& th : threads) th.join();
        auto t1 = std::chrono::steady_clock::now();

        // Render
        uint32_t fi = swapchain->GetCurrentBackBufferIndex();
        cmdAlloc->Reset();
        cmdList->Reset(cmdAlloc.Get(), pso.Get());
        cmdList->SetGraphicsRootSignature(rootSig.Get());
        ID3D12DescriptorHeap* heaps[] = { srvHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootDescriptorTable(0, srvHeap->GetGPUDescriptorHandleForHeapStart());

        D3D12_RESOURCE_BARRIER b = {}; b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = renderTargets[fi].Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        cmdList->ResourceBarrier(1, &b);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += fi * rtvSize;
        float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

        D3D12_VIEWPORT vp = { 0, 0, (float)WIDTH, (float)HEIGHT, 0, 1 };
        D3D12_RECT sr = { 0, 0, (LONG)WIDTH, (LONG)HEIGHT };
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sr);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(6, SPRITE_COUNT, 0, 0);

        b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &b);
        cmdList->Close();

        ID3D12CommandList* lists[] = { cmdList.Get() };
        queue->ExecuteCommandLists(1, lists);
        auto t2 = std::chrono::steady_clock::now();

        swapchain->Present(0, 0);
        fenceValue++;
        queue->Signal(fence.Get(), fenceValue);
        if (fence->GetCompletedValue() < fenceValue) {
            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        auto t3 = std::chrono::steady_clock::now();

        totalUpdate += std::chrono::duration<double, std::milli>(t1 - t0).count();
        totalRender += std::chrono::duration<double, std::milli>(t2 - t1).count();
        totalPresent += std::chrono::duration<double, std::milli>(t3 - t2).count();
        frameCount++;

        if (frameCount % 100 == 0) {
            auto elapsed = std::chrono::duration<double>(t3 - startTime).count();
            double fps = frameCount / elapsed;
            printf("Frame %u: FPS=%.1f  Update=%.2fms  Render=%.2fms  Present=%.2fms\n",
                frameCount, fps, totalUpdate / frameCount, totalRender / frameCount, totalPresent / frameCount);
        }
    }
done:
    fenceValue++;
    queue->Signal(fence.Get(), fenceValue);
    if (fence->GetCompletedValue() < fenceValue) {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    spriteBuf->Unmap(0, nullptr);
    CloseHandle(fenceEvent);

    double totalElapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    printf("\n=== Final Results ===\n");
    printf("Total frames: %u in %.1fs\n", frameCount, totalElapsed);
    printf("Average FPS: %.1f\n", frameCount / totalElapsed);
    printf("Avg Update: %.2fms  Avg Render: %.2fms  Avg Present: %.2fms\n",
        totalUpdate / frameCount, totalRender / frameCount, totalPresent / frameCount);
    return 0;
}
