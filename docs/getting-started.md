# Cacao RHI Getting Started Guide

## Introduction

Cacao is a cross-backend graphics Rendering Hardware Interface (RHI) providing a unified C++ API for different GPU backends:

- **DirectX 12** — Primary backend on Windows
- **Vulkan** — Cross-platform backend
- **Metal** — macOS/iOS backend
- **WebGPU** — Web and cross-platform backend (Dawn/wgpu-native)
- **DirectX 11** / **OpenGL** — Compatibility backends

Cacao supports rasterization pipelines, compute pipelines, and ray tracing pipelines, using Slang as the shading language.

## Requirements

| Dependency | Minimum Version |
|------------|----------------|
| Windows | 10/11 |
| MSVC | Visual Studio 2022 |
| CMake | 3.20+ |
| Vulkan SDK | 1.3+ (required for Vulkan backend) |

## Building

```bash
# Configure (Ninja recommended)
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build all targets
cmake --build cmake-build-debug

# Build a single demo
cmake --build cmake-build-debug --target HelloTriangle
```

## Core Concepts

### Instance

The entry point of Cacao, responsible for backend initialization and global state management.

```cpp
InstanceCreateInfo info;
info.type = BackendType::DirectX12;
info.applicationName = "MyApp";
info.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
auto instance = Instance::Create(info);
```

### Adapter

A GPU adapter representing a physical graphics card. Obtained via `Instance::EnumerateAdapters()`.

```cpp
auto adapter = instance->EnumerateAdapters().front();
std::cout << "GPU: " << adapter->GetProperties().name << std::endl;
```

### Device

A logical device that serves as the factory for creating all GPU resources.

```cpp
DeviceCreateInfo deviceCI;
deviceCI.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
deviceCI.CompatibleSurface = surface;
auto device = adapter->CreateDevice(deviceCI);
```

### Surface & Swapchain

Surface connects the window system to the graphics backend; Swapchain manages multiple back buffers for tear-free frame presentation.

```cpp
auto swapchain = device->CreateSwapchain(
    SwapchainBuilder()
    .SetExtent(surfaceCaps.currentExtent)
    .SetFormat(Format::BGRA8_UNORM)
    .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
    .SetPresentMode(PresentMode::Fifo)  // V-Sync
    .SetMinImageCount(imageCount)
    .SetSurface(surface)
    .Build());
```

### CommandBufferEncoder

A command encoder that records GPU commands (draw, compute, copy, barriers, etc.).

```cpp
auto cmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
cmd->Begin({true});
// ... record commands ...
cmd->End();
graphicsQueue->Submit(cmd, sync, frame);
```

### GraphicsPipeline

A graphics pipeline state object that encapsulates shaders, vertex format, rasterization settings, blend modes, etc.

```cpp
auto pipeline = device->CreateGraphicsPipeline(
    GraphicsPipelineBuilder()
    .SetShaders({vs, fs})
    .AddVertexBinding(0, sizeof(Vertex))
    .AddVertexAttribute(0, 0, Format::RGB32_FLOAT, 0)
    .SetTopology(PrimitiveTopology::TriangleList)
    .SetCullMode(CullMode::None)
    .AddColorFormat(swapchain->GetFormat())
    .AddColorAttachmentDefault(false)
    .SetLayout(pipelineLayout)
    .Build());
```

### DescriptorSet

A resource binding set that binds Buffers, Textures, Samplers, and other resources to shader slots in the pipeline.

```cpp
auto layout = device->CreateDescriptorSetLayout(
    DescriptorSetLayoutBuilder()
    .AddBinding(0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex)
    .Build());

auto pool = device->CreateDescriptorPool(
    DescriptorPoolBuilder()
    .SetMaxSets(framesInFlight)
    .AddPoolSize(DescriptorType::UniformBuffer, framesInFlight)
    .Build());

auto descSet = pool->AllocateDescriptorSet(layout);
descSet->WriteBuffer({0, uniformBuffer, 0, sizeof(MVPData), bufSize, DescriptorType::UniformBuffer, 0});
descSet->Update();
```

## Basic Frame Rendering Flow

```
1. sync->WaitForFrame(frame)        // Wait for GPU to finish old frame
2. swapchain->AcquireNextImage()     // Acquire available back buffer
3. sync->ResetFrameFence(frame)      // Reset fence
4. cmd->Begin()                      // Begin recording commands
5. TransitionImage → ColorAttachment // Resource state transition
6. BeginRendering()                  // Begin render pass
7. SetViewport / SetScissor          // Set render area
8. BindPipeline / BindVertexBuffer   // Bind resources
9. Draw / DrawIndexed                // Issue draw calls
10. EndRendering()                   // End render pass
11. TransitionImage → Present        // Prepare for presentation
12. cmd->End()                       // End recording
13. queue->Submit(cmd, sync, frame)  // Submit to GPU
14. swapchain->Present()             // Display on screen
```

## Demo Overview

| Demo | Description | Key Features |
|------|-------------|-------------|
| **HelloTriangle** | Colored triangle | Minimal rasterization, vertex color interpolation |
| **Scene3D** | Rotating colored cube | MVP matrices, UniformBuffer, indexed drawing |
| **TexturedQuad** | Textured quad | Texture creation, Sampler, SampledImage |
| **ComputeDemo** | Compute pipeline | Compute Shader, SSBO |
| **ShadowMap** | Shadow mapping | Multi-pass rendering, depth texture |
| **CornellBox** | Path tracing | Ray tracing pipeline, acceleration structures, SBT |

## Writing Shaders (Slang)

Cacao uses [Slang](https://shader-slang.com/) as its shading language, which is highly compatible with HLSL syntax.

### Entry Point Attributes

```hlsl
[shader("vertex")]
VSOutput mainVS(VSInput input) { ... }

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target { ... }
```

### Vertex Semantics

The `semanticName` parameter in `AddVertexAttribute` on the C++ side defaults to `"TEXCOORD"`. To customize:

```cpp
// Specify POSITION semantic on C++ side
.AddVertexAttribute(0, 0, Format::RGB32_FLOAT, 0, "POSITION", 0)
```

```hlsl
// Match in shader
float3 position : POSITION;
```

### Resource Bindings

```hlsl
// Uniform Buffer (cbuffer)
[[vk::binding(0, 0)]]
cbuffer MVPBuffer : register(b0) { float4x4 mvp; };

// Storage Buffer
[[vk::binding(0, 0)]]
StructuredBuffer<MyStruct> data : register(t0);

// Texture + Sampler
[[vk::binding(1, 0)]]
Texture2D myTexture : register(t1);
[[vk::binding(2, 0)]]
SamplerState mySampler : register(s2);
```

### Matrix Convention

Cacao uses column-major matrices. In Slang, use row-vector × matrix multiplication:

```hlsl
output.position = mul(float4(pos, 1.0), mvpMatrix);
```

## Cross-Backend Notes

| Item | DX12 | Vulkan | Metal | WebGPU |
|------|------|--------|-------|--------|
| Swapchain Format | `BGRA8_UNORM` | `BGRA8_UNORM` / `RGBA8_UNORM` | `BGRA8_UNORM` | `BGRA8_UNORM` |
| Shader Bytecode | DXIL | SPIR-V | MSL (Metal Shading Language) | WGSL |
| Y-axis Direction | Unified by framework | Unified by framework | Unified by framework | Unified by framework |
| Validation | D3D12 Debug Layer | Vulkan Validation Layer | Metal API Validation | WebGPU Error Callback |
| Backend Selection | `BackendType::DirectX12` | `BackendType::Vulkan` | `BackendType::Metal` | `BackendType::WebGPU` |
| Ray Tracing | DXR 1.1 | VK_KHR_ray_tracing | Metal RT (macOS 11+) | Not supported |
| Platform | Windows | Windows/Linux/macOS | macOS/iOS | All (via Dawn) |

Cacao is designed to make application code completely backend-agnostic. Simply choose the backend in `InstanceCreateInfo::type`. Shaders are written in Slang, and the framework automatically compiles them to the appropriate bytecode for the selected backend.
