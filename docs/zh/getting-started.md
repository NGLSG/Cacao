# 快速入门

## 环境要求

- **C++ 20** 编译器 (MSVC 2022+, GCC 13+, Clang 16+)
- **CMake 4.0+**
- **Vulkan SDK** (可选, Vulkan 后端)
- **Windows SDK** (DX12/DX11 后端)
- **Xcode** (可选, Metal 后端, macOS/iOS)
- **Dawn 或 wgpu-native** (可选, WebGPU 后端)
- **Python 3** (OpenGL glad 加载器生成)

## 构建

```bash
git clone <repo-url> Cacao && cd Cacao
mkdir build && cd build
cmake .. \
  -DCACAO_BACKEND_VULKAN=ON \
  -DCACAO_BACKEND_D3D12=ON \
  -DCACAO_BACKEND_D3D11=ON \
  -DCACAO_BACKEND_OPENGL=ON
cmake --build . --target Test -j
```

Slang 着色器编译器通过 CPM 自动下载。

## 运行

通过环境变量选择后端:

```bash
# CMD
set CACAO_BACKEND=d3d12
Test.exe

# PowerShell
$env:CACAO_BACKEND = "vulkan"
.\Test.exe
```

支持: `vulkan`, `d3d12`, `dx12`, `d3d11`, `dx11`, `opengl`, `gl`, `metal`, `webgpu`

## 快速上手（EasyWrapper）

使用 `CacaoEasy.h` 快速原型开发：

```cpp
#include "CacaoEasy.h"
using namespace Cacao;

int main() {
    // 1. 创建上下文（自动选择最佳后端）
    auto ctx = EasyContext::Create({
        .appName = "MyApp",
        .width = 1280, .height = 720,
        .windowHandle = {.hWnd = hwnd, .hInst = hInstance}
    });

    // 2. 一行创建管线
    auto qp = ctx->CreateQuickPipeline("sprite.slang", "mainVS", "mainPS", {
        {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex},
        {1, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
        {2, DescriptorType::Sampler, 1, ShaderStage::Fragment}
    });

    // 3. 创建缓冲区
    auto spriteBuffer = ctx->CreateStorageBuffer(spriteDataSize, spriteData);

    // 4. 渲染循环
    while (running) {
        auto cmd = ctx->BeginFrame();

        ctx->ClearAndBeginRendering(cmd, 0.1f, 0.1f, 0.2f);
        cmd->BindGraphicsPipeline(qp.pipeline);
        cmd->Draw(6, spriteCount, 0, 0);
        ctx->EndRendering(cmd);

        ctx->EndFrame(cmd);
    }
}
```

`EasyContext` 自动处理：后端选择、设备/交换链创建、同步、着色器编译、帧索引管理、资源转换、视口/裁剪设置。

## 最简三角形（EasyWrapper）

```cpp
#include "CacaoEasy.h"
using namespace Cacao;

// triangle.slang:
// [shader("vertex")]
// float4 mainVS(uint vid : SV_VertexID) : SV_Position {
//     float2 positions[3] = {float2(0, 0.5), float2(-0.5, -0.5), float2(0.5, -0.5)};
//     return float4(positions[vid], 0, 1);
// }
// [shader("fragment")]
// float4 mainPS() : SV_Target { return float4(1, 0.5, 0, 1); }

int main() {
    auto ctx = EasyContext::Create({.width=800, .height=600, .windowHandle=hwnd});
    auto qp = ctx->CreateQuickPipeline("triangle.slang");

    while (running) {
        auto cmd = ctx->BeginFrame();
        ctx->ClearAndBeginRendering(cmd, 0, 0, 0);
        cmd->BindGraphicsPipeline(qp.pipeline);
        cmd->Draw(3, 1, 0, 0);
        ctx->EndRendering(cmd);
        ctx->EndFrame(cmd);
    }
}
```

10 行 C++ 代码，在任意后端渲染一个三角形。

## 第一个三角形（底层 API）

```cpp
#include "Cacao.hpp"
using namespace Cacao;

int main() {
    // 1. 创建实例
    InstanceCreateInfo ici;
    ici.type = BackendType::DirectX12;
    ici.applicationName = "MyApp";
    auto instance = Instance::Create(ici);

    // 2. 选择 GPU
    auto adapter = instance->EnumerateAdapters().front();

    // 3. 创建设备
    DeviceCreateInfo dci;
    dci.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    auto device = adapter->CreateDevice(dci);

    // 4. 创建交换链 (需要窗口句柄)
    auto surface = instance->CreateSurface(windowHandle);
    auto swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(1280, 720)
        .SetFormat(Format::BGRA8_UNORM)
        .SetSurface(surface)
        .Build());

    // 5. 编译着色器 (Slang)
    auto compiler = instance->CreateShaderCompiler();
    ShaderCreateInfo sci;
    sci.SourcePath = "triangle.slang";
    sci.EntryPoint = "mainVS";
    sci.Stage = ShaderStage::Vertex;
    auto vs = compiler->CompileOrLoad(device, sci);
    sci.EntryPoint = "mainPS";
    sci.Stage = ShaderStage::Fragment;
    auto ps = compiler->CompileOrLoad(device, sci);

    // 6. 创建管线
    auto pipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .SetShaders({vs, ps})
        .SetTopology(PrimitiveTopology::TriangleList)
        .AddColorFormat(swapchain->GetFormat())
        .Build());

    // 7. 渲染循环
    auto cmd = device->CreateCommandBufferEncoder();
    auto queue = device->GetQueue(QueueType::Graphics, 0);
    auto sync = device->CreateSynchronization(swapchain->GetImageCount());

    while (running) {
        sync->WaitForFrame(frame);
        int imageIndex;
        swapchain->AcquireNextImage(sync, frame, imageIndex);

        cmd->Reset(); cmd->Begin({true});
        // ... 录制命令 ...
        cmd->Draw(3, 1, 0, 0);
        cmd->End();

        queue->Submit(cmd, sync, frame);
        swapchain->Present(queue, sync, frame);
        frame = (frame + 1) % swapchain->GetImageCount();
    }
}
```
