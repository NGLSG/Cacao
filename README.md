# Cacao

A multi-backend GPU Rendering Hardware Interface (RHI) abstraction layer for C++20.

## Supported Backends

| Backend | Status | Notes |
|---------|--------|-------|
| Vulkan | Ready | Primary backend, Vulkan 1.3+ |
| DirectX 12 | Ready | Windows 10+, Feature Level 12.0 |
| DirectX 11 | Ready | Windows 7+, Feature Level 11.0 |
| OpenGL 4.5 | Ready | Desktop GL 4.5 Core |
| Metal | Ready | macOS 10.15+ / iOS 13+ |
| WebGPU | Ready | Dawn / wgpu-native |

## Features

- Unified device, queue, and swapchain abstraction across all backends
- Descriptor set / pipeline layout / root signature mapping
- Slang shader compiler integration (cross-compiles to SPIR-V, DXIL, DXBC, GLSL, WGSL, MSL)
- Ray tracing pipeline support (Vulkan, DX12, Metal)
- Multi-threaded command buffer recording
- Builder pattern API for pipeline and resource creation
- Vertex-buffer-free instanced rendering
- Query pools (timestamp, occlusion, pipeline statistics)
- Timeline semaphores
- Staging buffer pools with ring allocation
- Debug markers (PIX, RenderDoc, GL debug groups)
- MSAA support with resolve

## Demo Examples

| Demo | Description |
|------|-------------|
| HelloTriangle | Minimal colored triangle (rasterization) |
| Scene3D | Rotating cube with MVP matrices and indexed drawing |
| TexturedQuad | Texture loading, sampling, and rendering |
| ComputeDemo | Compute shader with SSBO |
| ShadowMap | Multi-pass rendering with depth textures |
| CornellBox | Path tracing with RT pipeline and acceleration structures |

## Dependencies

- **Vulkan SDK** (optional, for Vulkan backend)
- **Slang** shader compiler (auto-fetched via CPM)
- **glad** OpenGL loader (bundled in `third_party/glad`)
- **Windows SDK** (for DX11/DX12 backends)
- **Dawn / wgpu-native** (optional, for WebGPU backend)
- **Xcode** (optional, for Metal backend on macOS/iOS)

## Building

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Build a specific demo
cmake --build build --target HelloTriangle
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

Valid values: `vulkan`, `d3d12`, `d3d11`, `opengl`, `metal`, `webgpu`

## Quick Start

```cpp
#include "Cacao.hpp"
using namespace Cacao;

int main() {
    // 1. Create instance
    InstanceCreateInfo ici;
    ici.type = BackendType::DirectX12;
    ici.applicationName = "MyApp";
    ici.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
    auto instance = Instance::Create(ici);

    // 2. Pick GPU
    auto adapter = instance->EnumerateAdapters().front();

    // 3. Create device
    DeviceCreateInfo dci;
    dci.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
    dci.CompatibleSurface = surface;
    auto device = adapter->CreateDevice(dci);

    // 4. Create swapchain
    auto swapchain = device->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(surfaceCaps.currentExtent)
        .SetFormat(Format::BGRA8_UNORM)
        .SetSurface(surface)
        .Build());

    // 5. Compile shaders (Slang)
    auto compiler = instance->CreateShaderCompiler();
    ShaderCreateInfo sci;
    sci.SourcePath = "hello_triangle.slang";
    sci.EntryPoint = "mainVS";
    sci.Stage = ShaderStage::Vertex;
    auto vs = compiler->CompileOrLoad(device, sci);
    sci.EntryPoint = "mainPS";
    sci.Stage = ShaderStage::Fragment;
    auto fs = compiler->CompileOrLoad(device, sci);

    // 6. Create pipeline
    auto pipeline = device->CreateGraphicsPipeline(
        GraphicsPipelineBuilder()
        .SetShaders({vs, fs})
        .SetTopology(PrimitiveTopology::TriangleList)
        .AddColorFormat(swapchain->GetFormat())
        .AddColorAttachmentDefault(false)
        .SetLayout(device->CreatePipelineLayout(PipelineLayoutBuilder().Build()))
        .Build());

    // 7. Render loop
    auto sync = device->CreateSynchronization(framesInFlight);
    auto queue = device->GetQueue(QueueType::Graphics, 0);

    while (running) {
        sync->WaitForFrame(frame);
        int imageIndex;
        swapchain->AcquireNextImage(sync, frame, imageIndex);
        sync->ResetFrameFence(frame);

        auto cmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
        cmd->Begin({true});
        cmd->TransitionImage(backBuffer, ImageTransition::PresentToColorAttachment);
        cmd->BeginRendering(renderInfo);
        cmd->BindGraphicsPipeline(pipeline);
        cmd->Draw(3, 1, 0, 0);
        cmd->EndRendering();
        cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
        cmd->End();

        queue->Submit(cmd, sync, frame);
        swapchain->Present(presentQueue, sync, frame);
        frame = (frame + 1) % framesInFlight;
    }
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
    Metal/            Metal backend (macOS/iOS)
    WebGPU/           WebGPU backend (Dawn/wgpu-native)
src/                  Implementation files
demos/                Demo examples (HelloTriangle, Scene3D, etc.)
docs/                 Documentation (English and Chinese)
third_party/
  glad/               OpenGL loader
  D3D12MA/            D3D12 Memory Allocator
```

## Documentation

- [Getting Started (English)](docs/getting-started.md)
- [Getting Started (中文)](docs/zh/getting-started.md)
- [API Reference](docs/api-reference.md)
- [Architecture](docs/architecture.md)
- [Backend Notes](docs/backend-notes.md)
- [Shader Guide](docs/shader-guide.md)

## License

See LICENSE file.
