# Backend-Specific Notes

Cacao abstracts away backend differences, but there are important behaviors and limitations to be aware of for each backend.

## Vulkan

**CMake option**: `CACAO_BACKEND_VULKAN=ON` (default ON)
**Requires**: Vulkan SDK 1.3+

### Features

- Full feature support across the entire Cacao API
- Dynamic rendering (no render pass objects) via `VK_KHR_dynamic_rendering`
- Timeline semaphores via `VK_KHR_timeline_semaphore`
- Ray tracing via `VK_KHR_ray_tracing_pipeline` + `VK_KHR_acceleration_structure`
- Mesh shaders via `VK_EXT_mesh_shader`
- Bindless descriptors via `VK_EXT_descriptor_indexing`
- Pipeline cache serialization for faster startup
- Validation layers for debugging (enable with `InstanceFeature::ValidationLayer`)
- Debug markers for GPU profiling tools (RenderDoc, NSight)
- Buffer device address (BDA) for GPU pointers
- Async compute and transfer queues when hardware supports them

### Vulkan-Specific Behavior

- **Memory management**: Uses VMA (Vulkan Memory Allocator) internally for efficient GPU memory allocation
- **Shader format**: Compiles to SPIR-V via Slang
- **Image layout transitions**: Handled automatically by `TransitionImage()` and `PipelineBarrier()`
- **Queue family ownership**: Barriers include queue family transfer fields for multi-queue usage
- **Staging buffers**: Required for GPU-only buffer/texture uploads; use `BufferMemoryUsage::CpuToGpu` for staging

### Best Practices (Vulkan)

- Use `PresentMode::Mailbox` for lowest latency without tearing
- Enable validation layers during development, disable in release
- Use pipeline caches to reduce shader compilation stalls
- Prefer `BufferMemoryUsage::GpuOnly` for static data with separate staging uploads
- Use timeline semaphores for complex multi-queue synchronization

---

## DirectX 12

**CMake option**: `CACAO_BACKEND_D3D12=ON` (default ON on Windows)
**Requires**: Windows SDK 10.0.19041+, Windows 10 1903+

### Features

- Near-complete feature parity with Vulkan backend
- Ray tracing via DXR 1.1
- Mesh shaders via DirectX Mesh Shader API
- Bindless descriptors via shader-visible descriptor heaps
- Pipeline cache via PSO serialization
- GPU timestamps and pipeline statistics queries
- Debug layer for validation (enable with `InstanceFeature::ValidationLayer`)
- D3D12 Memory Allocator (D3D12MA) for efficient memory management
- Timeline fences for CPU-GPU synchronization

### DX12-Specific Behavior

- **Root signatures**: `PipelineLayout` maps to D3D12 root signatures internally
- **Descriptor heaps**: Cacao manages shader-visible CBV/SRV/UAV and sampler heaps automatically
- **Resource barriers**: `TransitionImage()` and `TransitionBuffer()` map to `D3D12_RESOURCE_BARRIER`
- **Shader format**: Compiles to DXIL via Slang
- **Swapchain**: Uses DXGI swap chains with `FLIP_DISCARD` model
- **Command allocators**: Managed per-frame by Cacao's command buffer pool

### DX12 Texture Upload

Row pitch alignment (256 bytes) is automatically handled. When uploading texture data via staging buffer, set `BufferRowLength` in `BufferImageCopy` to account for padding:

```cpp
uint32_t uploadRowPitch = (naturalRowPitch + 255) & ~255;
region.BufferRowLength = uploadRowPitch / bytesPerPixel;
```

### Best Practices (DX12)

- Let Cacao handle descriptor heap management; avoid raw native access
- Use `CommandBufferType::Primary` for main rendering; secondary command buffers map to bundles
- The debug layer (`InstanceFeature::ValidationLayer`) enables D3D12 debug validation + DRED for crash diagnostics
- Prefer `PresentMode::Fifo` for reliable vsync on Windows

---

## DirectX 11

**CMake option**: `CACAO_BACKEND_D3D11=ON` (default ON on Windows)
**Requires**: Windows SDK, Windows 7+

### Features

- Basic rendering support (graphics pipeline, compute)
- Immediate-mode command submission (no explicit command buffers internally)
- Compute shaders
- Texture sampling, render targets, depth buffers

### DX11 Limitations

| Feature | Status | Notes |
|---|---|---|
| Ray Tracing | Not supported | DX11 lacks DXR |
| Mesh Shaders | Not supported | DX11 lacks mesh shader API |
| Bindless Descriptors | Not supported | Limited descriptor model |
| Timeline Semaphore | Not supported | DX11 has simpler sync model |
| Pipeline Cache | Not supported | No PSO serialization |
| Buffer Device Address | Not supported | No BDA equivalent |
| Multi-Draw Indirect Count | Not supported | `DrawIndirectCount` is no-op |
| Independent Blend | Limited | Per-RT blend state limitations |
| Debug Markers | Limited | Basic annotation support |
| Async Compute | Not supported | Single immediate context |

### DX11-Specific Behavior

- **Immediate context**: All commands execute immediately on the device context; `CommandBufferEncoder` wraps deferred command lists
- **Shader format**: Compiles to DXBC via Slang
- **Resource binding**: Uses traditional slot-based binding (SRV/UAV/CBV/Sampler slots)
- **Dynamic rendering**: Emulated via `OMSetRenderTargets`; `BeginRendering`/`EndRendering` manage render target binding
- **Barriers**: Most barrier calls are no-ops since DX11 handles synchronization implicitly

### Best Practices (DX11)

- Use DX11 as a fallback for older hardware
- Avoid relying on advanced features (ray tracing, mesh shaders, bindless)
- Test with DX11 backend if targeting Windows 7/8 compatibility
- Compute shader support is available but without atomic buffer operations

---

## OpenGL

**CMake option**: `CACAO_BACKEND_OPENGL=ON` (default OFF)
**Requires**: OpenGL 4.5+, GLAD loader (bundled)

### Features

- Graphics pipeline with modern OpenGL (DSA, direct state access)
- Compute shaders (OpenGL 4.3+)
- Texture sampling, framebuffer objects
- Basic query support (timestamps, occlusion)
- MSAA support
- Pipeline cache via program binary

### OpenGL-Specific Behavior

- **Shader format**: Compiles to GLSL via Slang
- **No explicit sync**: OpenGL driver manages most synchronization; fence/barrier calls are mapped to `glMemoryBarrier` and `glFenceSync`
- **Descriptor sets**: Emulated via texture/buffer binding points; `DescriptorSet` updates translate to `glBindTextureUnit`, `glBindBufferRange`, etc.
- **Dynamic rendering**: Emulated with FBO management; `BeginRendering`/`EndRendering` create and bind framebuffer objects
- **Command buffers**: All commands execute immediately on the GL context; command buffer recording is deferred-then-replayed
- **Timeline semaphore**: Emulated with `glFenceSync` + polling

### OpenGL Limitations

| Feature | Status | Notes |
|---|---|---|
| Ray Tracing | Not supported | No GL ray tracing extensions |
| Mesh Shaders | Not supported | `GL_NV_mesh_shader` not used |
| Bindless Descriptors | Limited | `GL_ARB_bindless_texture` where available |
| Pipeline Cache | Limited | Program binary save/restore |
| Buffer Device Address | Not supported | No BDA equivalent |
| Separate Sampler | Supported | Via `glBindSampler` |
| Storage Buffer | Supported | SSBO (GL 4.3+) |
| Compute Shader | Supported | GL 4.3+ |
| Multi-Draw Indirect | Supported | `glMultiDrawIndirect` |

### Best Practices (OpenGL)

- Enable `CACAO_BACKEND_OPENGL` only when Vulkan is unavailable
- GLSL generation via Slang handles most cross-compilation
- Be aware of GL context threading constraints (single-thread by default)
- Use `glDebugMessageCallback` for validation (enabled with `InstanceFeature::ValidationLayer`)

---

## OpenGL ES

**CMake option**: `CACAO_BACKEND_OPENGLES=ON` (default OFF)
**Requires**: OpenGL ES 3.2+, EGL

Same as OpenGL backend with additional restrictions:
- No geometry/tessellation shaders
- No multi-draw indirect
- More limited texture format support
- Compute shader support requires ES 3.1+

---

## Backend Selection Strategy

| Scenario | Recommended Backend |
|---|---|
| Windows desktop (modern GPU) | Vulkan or DX12 |
| Windows desktop (older GPU) | DX11 |
| Linux desktop | Vulkan |
| macOS/iOS | Metal (planned) |
| Android | Vulkan or OpenGL ES |
| Cross-platform (maximum compat) | Vulkan with OpenGL fallback |
| Ray tracing required | Vulkan or DX12 |
| Simplest debugging | DX12 with PIX, or Vulkan with RenderDoc |

## Native Handle Access

For advanced use cases, access the underlying backend handles:

```cpp
cmd->ExecuteNative([](void* nativeHandle) {
    // Vulkan: nativeHandle is VkCommandBuffer
    // DX12:   nativeHandle is ID3D12GraphicsCommandList*
    // DX11:   nativeHandle is ID3D11DeviceContext*
    // OpenGL: nativeHandle is context pointer
});

void* handle = cmd->GetNativeHandle();
```

> **Warning**: Using native handles bypasses Cacao's state tracking. Use with caution and restore state after native calls.

## Compile-Time Backend Detection

```cpp
#ifdef CACAO_BACKEND_VULKAN
    // Vulkan-specific code
#endif

#ifdef CACAO_BACKEND_D3D12
    // DX12-specific code
#endif

#ifdef CACAO_BACKEND_D3D11
    // DX11-specific code
#endif

#ifdef CACAO_BACKEND_OPENGL
    // OpenGL-specific code
#endif

#ifdef CACAO_BACKEND_METAL
    // Metal-specific code
#endif

#ifdef CACAO_BACKEND_WEBGPU
    // WebGPU-specific code
#endif
```

These macros are defined by CMake and propagated via `target_compile_definitions`.

## Metal

**CMake option**: `CACAO_BACKEND_METAL=ON`
**Requires**: macOS 10.15+ / iOS 13+, Xcode

### Features

- Native Apple GPU support with optimal performance
- Ray tracing on Apple Silicon (macOS 11+) via Metal RT
- Automatic resource hazard tracking (no manual barriers needed)
- Tile-based deferred rendering optimization on Apple Silicon
- Metal Performance Shaders for optimized operations
- Shader compilation to Metal Shading Language (MSL) via Slang

### Metal-Specific Behavior

- **No explicit barriers**: Metal tracks resource usage automatically; `TransitionImage()` and `PipelineBarrier()` are no-ops
- **Command buffer model**: Uses `MTLCommandBuffer` from `MTLCommandQueue`; command buffers are single-use
- **Descriptor model**: Metal uses argument buffers instead of descriptor sets; Cacao maps DescriptorSet to argument buffers
- **Shader format**: Compiles to Metal IR via Slang, then to GPU binary at runtime
- **Memory model**: Metal manages memory automatically with `MTLResourceOptions` mapping from `BufferMemoryUsage`
- **Swapchain**: Uses `CAMetalLayer` for drawable management
- **Push constants**: Not natively supported; emulated via inline uniform buffers

### Limitations

- macOS/iOS only
- No geometry/tessellation shaders
- Limited compute shader shared memory compared to Vulkan/DX12
- Pipeline cache serialization not exposed

## WebGPU

**CMake option**: `CACAO_BACKEND_WEBGPU=ON`
**Requires**: Dawn or wgpu-native

### Features

- Cross-platform (Windows, macOS, Linux, Web)
- Automatic resource state tracking (no manual barriers)
- Safe API with validation built-in
- Async operations with callback model
- Compatible with web browsers via Emscripten

### WebGPU-Specific Behavior

- **No explicit barriers**: WebGPU handles all resource transitions automatically; barrier functions are no-ops
- **No push constants**: Must use uniform buffers instead
- **Bind group model**: WebGPU uses bind groups (mapped from DescriptorSet) and bind group layouts
- **Shader format**: Compiles to WGSL via Slang
- **Buffer mapping**: Async-only mapping model; Cacao provides synchronous wrappers
- **No buffer device address**: Not exposed in WebGPU
- **Queue model**: Single queue per device; all operations go through the same queue
- **Error handling**: Uses uncaptured error callback for validation errors

### Limitations

- No ray tracing support
- No mesh shaders
- No geometry/tessellation shaders
- Limited storage buffer features
- No pipeline statistics queries
- Max bind groups limited (typically 4)
