# Architecture

## Abstraction Layers

```
┌─────────────────────────────┐
│        Application          │
│    (main.cpp / Engine)      │
├─────────────────────────────┤
│      Cacao RHI API          │
│  Instance / Device / Queue  │
│  Buffer / Texture / Sampler │
│  DescriptorSet / Pipeline   │
│  CommandBuffer / Swapchain  │
│  Synchronization / Query    │
├──────┬──────┬──────┬────────┤
│  VK  │ DX12 │ DX11 │   GL   │
│Backend│Backend│Backend│Backend │
├──────┴──────┴──────┴────────┤
│       Slang Compiler        │
│   (SPIRV/DXIL/DXBC/GLSL)   │
├─────────────────────────────┤
│      GPU Hardware           │
└─────────────────────────────┘
```

## Backend Mapping Table

| RHI Concept | Vulkan | DX12 | DX11 | OpenGL |
|-------------|--------|------|------|--------|
| **Instance** | VkInstance | IDXGIFactory6 | IDXGIFactory1 | GL Context (glad) |
| **Adapter** | VkPhysicalDevice | IDXGIAdapter4 | IDXGIAdapter1 | GL_RENDERER string |
| **Device** | VkDevice | ID3D12Device5 | ID3D11Device5 | GL Context |
| **Queue** | VkQueue | ID3D12CommandQueue | ID3D11DeviceContext | glFinish (implicit) |
| **CommandBuffer** | VkCommandBuffer | ID3D12GraphicsCommandList | Deferred Context | Lambda list |
| **Swapchain** | VkSwapchainKHR | IDXGISwapChain4 | IDXGISwapChain | SwapBuffers(HDC) |
| **Buffer** | VkBuffer + VMA | ID3D12Resource (heap) | ID3D11Buffer | GLuint (glGenBuffers) |
| **Texture** | VkImage + VMA | ID3D12Resource | ID3D11Texture2D | GLuint (glGenTextures) |
| **TextureView** | VkImageView | D3D12_CPU_DESCRIPTOR_HANDLE | ID3D11ShaderResourceView | GLuint (same texture) |
| **Sampler** | VkSampler | D3D12_SAMPLER_DESC | ID3D11SamplerState | GLuint (glGenSamplers) |
| **DescriptorSetLayout** | VkDescriptorSetLayout | Root Signature params | Binding slot metadata | Binding info |
| **DescriptorSet** | VkDescriptorSet | Descriptor heap range | Slot bindings | GLBindingGroup |
| **DescriptorPool** | VkDescriptorPool | Descriptor heap | N/A (returns null) | N/A |
| **PipelineLayout** | VkPipelineLayout | ID3D12RootSignature | N/A (returns null) | N/A |
| **GraphicsPipeline** | VkPipeline | ID3D12PipelineState | ID3D11 state objects | GLuint program + state |
| **ComputePipeline** | VkPipeline | ID3D12PipelineState | ID3D11ComputeShader | GLuint program |
| **PipelineCache** | VkPipelineCache | ID3D12PipelineLibrary | N/A (empty) | glProgramBinary |
| **ShaderModule** | VkShaderModule (SPIRV) | DXIL bytecode | DXBC bytecode | GLuint shader (GLSL) |
| **Synchronization** | VkFence + VkSemaphore | ID3D12Fence | ID3D11Query (event) | GLsync |
| **TimelineSemaphore** | VkSemaphore (timeline) | ID3D12Fence | Event Query emulation | GLsync map |
| **QueryPool** | VkQueryPool | ID3D12QueryHeap | ID3D11Query[] | GLuint[] (glGenQueries) |
| **StagingBuffer** | VMA upload allocation | Upload Heap ring buffer | D3D11_USAGE_STAGING pool | Persistent mapped buffer |
| **Barrier** | VkPipelineBarrier | D3D12 ResourceBarrier | No-op | glMemoryBarrier |
| **Present** | vkQueuePresentKHR | IDXGISwapChain::Present | IDXGISwapChain::Present | SwapBuffers |

## Resource State Model

```
Undefined ──┬── RenderTarget ──── Present
            ├── DepthWrite
            ├── ShaderRead
            ├── CopySrc ──────── CopyDst
            └── General
```

- **Vulkan/DX12**: Explicit state transitions via barriers (GPU-enforced)
- **DX11/GL**: Implicit state management (driver handles hazards)
- **RHI rule**: Always call `TransitionImage()` — modern backends need it, legacy backends safely ignore it

## Command Recording Model

| Backend | Model | Threading |
|---------|-------|-----------|
| Vulkan | True GPU command list | Multi-threaded recording, single-threaded submit |
| DX12 | True GPU command list | Multi-threaded recording, single-threaded submit |
| DX11 | Immediate/Deferred context | Single-threaded (deferred context for recording) |
| GL | Lambda capture + deferred replay | Single-threaded, state isolated per Execute() |

## Shader Pipeline

```
.slang file
    │
    ▼ (Slang Compiler)
    ├── Vulkan: SPIR-V 1.5
    ├── DX12:   DXIL (SM 6.6)
    ├── DX11:   DXBC (SM 5.0)
    ├── GL:     GLSL 4.50 (+ Vulkan→GL fixup)
    ├── Metal:  MSL (planned)
    └── WebGPU: WGSL (planned)
```

## Backend Tiers

| Tier | Backends | Guarantees |
|------|----------|------------|
| **Tier 1** (Full) | Vulkan, DX12 | All RHI features, explicit barriers, multi-queue, pipeline cache, async compute, StorageBuffer RW in all stages |
| **Tier 2** (Compatible) | DX11, OpenGL, GLES | Core rendering features, single queue, implicit barriers, some features return nullptr/no-op |

### Tier 1 Guarantees
- True GPU command lists with multi-threaded recording
- Explicit resource state transitions (barriers enforced)
- Multiple command queues (graphics + compute + transfer)
- Pipeline cache serialization
- StorageBuffer read/write in all shader stages
- Descriptor set / pipeline layout with full binding validation
- Timeline semaphores with native GPU support

### Tier 2 Limitations
- Command buffers are emulated (DX11: immediate context, GL: lambda replay)
- Barriers are no-op (driver manages hazards)
- Single command queue only
- No pipeline cache serialization
- StorageBuffer may be read-only in vertex/fragment stages (DX11)
- PipelineLayout returns nullptr (not needed)
- Timeline semaphores emulated via event queries / GL sync

GL-specific shader fixups applied in `GLShaderModule`:
- `gl_VertexIndex` → `gl_VertexID`
- `gl_InstanceIndex` → `gl_InstanceID`
