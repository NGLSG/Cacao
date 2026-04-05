# Cacao

A multi-backend GPU Rendering Hardware Interface (RHI) abstraction layer for C++20.

## Supported Backends

| Backend | Status | Notes |
|---------|--------|-------|
| Vulkan | Ready | Primary backend, Vulkan 1.2+ |
| DirectX 12 | Ready | Windows 10+, Feature Level 12.0 |
| DirectX 11 | Ready | Windows 7+, Feature Level 11.0 |
| OpenGL 4.5 | Ready | Desktop GL 4.5 Core |
| OpenGL ES 3.2 | Ready | Shares GL backend code |
| Metal | Planned | macOS / iOS |
| WebGPU | Planned | Browser / Native |

## Features

- Unified device, queue, and swapchain abstraction across all backends
- Descriptor set / pipeline layout / root signature mapping
- Slang shader compiler integration (cross-compiles to SPIR-V, DXIL, DXBC, GLSL, WGSL, MSL)
- Multi-threaded command buffer recording
- Builder pattern API for pipeline and resource creation
- Vertex-buffer-free instanced rendering
- Query pools (timestamp, occlusion, pipeline statistics)
- Timeline semaphores
- Staging buffer pools with ring allocation
- Debug markers (PIX, RenderDoc, GL debug groups)
- MSAA support with resolve

## Dependencies

- **Vulkan SDK** (optional, for Vulkan backend)
- **Slang** shader compiler (auto-fetched via CPM)
- **glad** OpenGL loader (bundled in `third_party/glad`)
- **Windows SDK** (for DX11/DX12 backends)

## Building

```bash
cmake -B build \
  -DCACAO_BACKEND_VULKAN=ON \
  -DCACAO_BACKEND_D3D12=ON \
  -DCACAO_BACKEND_D3D11=ON \
  -DCACAO_BACKEND_OPENGL=ON
cmake --build build --target Test -j
```

## Running

Select a backend via the `CACAO_BACKEND` environment variable:

```bash
# Windows
set CACAO_BACKEND=d3d12
.\build\Test.exe

# PowerShell
$env:CACAO_BACKEND = "vulkan"
.\build\Test.exe
```

Valid values: `vulkan`, `d3d12`, `d3d11`, `opengl`

## Quick Start

```cpp
#include <Instance.h>
#include <Device.h>
#include <Swapchain.h>
#include <Pipeline.h>
#include <ShaderCompiler.h>
#include <Builders.h>

using namespace Cacao;

// 1. Create instance
auto instance = Instance::Create(BackendType::Vulkan);
instance->Initialize({.enableValidation = true});

// 2. Pick an adapter (GPU)
auto adapters = instance->EnumerateAdapters();
auto adapter = adapters[0];

// 3. Create device
DeviceCreateInfo deviceInfo;
deviceInfo.QueueRequests = {{QueueType::Graphics, 1}};
auto device = adapter->CreateDevice(deviceInfo);

// 4. Create swapchain
SwapchainCreateInfo swapInfo;
swapInfo.Extent = {1280, 720};
swapInfo.Format = Format::BGRA8_UNORM;
auto swapchain = device->CreateSwapchain(swapInfo);

// 5. Compile shaders (Slang -> backend-native)
auto compiler = ShaderCompiler::Create(BackendType::Vulkan);
auto vertShader = compiler->CompileOrLoad(device, {
    .SourcePath = "shaders/sprite.slang",
    .EntryPoint = "vertexMain",
    .Stage = ShaderStage::Vertex
});
auto fragShader = compiler->CompileOrLoad(device, {
    .SourcePath = "shaders/sprite.slang",
    .EntryPoint = "fragmentMain",
    .Stage = ShaderStage::Fragment
});

// 6. Create graphics pipeline
GraphicsPipelineCreateInfo pipelineInfo;
pipelineInfo.Shaders = {vertShader, fragShader};
pipelineInfo.ColorAttachmentFormats = {swapchain->GetFormat()};
pipelineInfo.Layout = device->CreatePipelineLayout({});
auto pipeline = device->CreateGraphicsPipeline(pipelineInfo);

// 7. Render loop
auto sync = device->CreateSynchronization(2);
auto queue = device->GetQueue(QueueType::Graphics);

for (uint32_t frame = 0; running; frame++) {
    uint32_t fi = frame % 2;
    sync->WaitForFrame(fi);

    int imageIndex;
    swapchain->AcquireNextImage(sync, fi, imageIndex);

    auto cmd = device->CreateCommandBufferEncoder();
    cmd->Begin();
    cmd->BeginRendering({
        .RenderArea = {0, 0, 1280, 720},
        .ColorAttachments = {{swapchain->GetBackBuffer(imageIndex)}}
    });
    cmd->BindGraphicsPipeline(pipeline);
    cmd->SetViewport({0, 0, 1280, 720});
    cmd->SetScissor({0, 0, 1280, 720});
    cmd->Draw(6, 10, 0, 0);
    cmd->EndRendering();
    cmd->End();

    queue->Submit(cmd, sync, fi);
    swapchain->Present(queue, sync, fi);
}
```

## Project Structure

```
include/              Public API headers
  Impls/
    Vulkan/           Vulkan backend
    D3D12/            DirectX 12 backend
    D3D11/            DirectX 11 backend
    OpenGL/           OpenGL / GLES backend
src/                  Implementation
third_party/glad/     OpenGL loader
```

## License

See LICENSE file.
