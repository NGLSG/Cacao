# Cacao RHI API Reference

## Smart Pointer Types

Cacao uses standard C++ smart pointers with aliases:

```cpp
template<typename T> using Ref = std::shared_ptr<T>;  // Shared ownership
template<typename T> using Box = std::unique_ptr<T>;   // Unique ownership
```

All API objects are returned as `Ref<T>`.

---

## Instance

Entry point for the Cacao RHI. Creates the backend context and enumerates available GPUs.

### BackendType

```cpp
enum class BackendType {
    Auto,        // Automatic selection (Vulkan > DX12 > DX11 > OpenGL)
    Vulkan,
    DirectX12,
    DirectX11,
    Metal,
    OpenGL,
    OpenGLES,
    WebGPU
};
```

### InstanceFeature

```cpp
enum class InstanceFeature {
    ValidationLayer,       // Enable debug validation
    Surface,               // Enable window surface support
    RayTracing,            // Enable ray tracing extensions
    MeshShader,            // Enable mesh shader support
    BindlessDescriptors    // Enable bindless resource access
};
```

### InstanceCreateInfo

```cpp
struct InstanceCreateInfo {
    BackendType type = BackendType::Auto;
    std::string applicationName;
    uint32_t appVersion = 1;
    std::vector<InstanceFeature> enabledFeatures;
};
```

### Instance Methods

| Method | Description |
|---|---|
| `static Ref<Instance> Create(const InstanceCreateInfo&)` | Create an instance for the specified backend |
| `BackendType GetType() const` | Get the active backend type |
| `std::vector<Ref<Adapter>> EnumerateAdapters()` | List all available GPUs |
| `bool IsFeatureEnabled(InstanceFeature) const` | Check if a feature is enabled |
| `Ref<Surface> CreateSurface(const NativeWindowHandle&)` | Create a window surface for presentation |
| `Ref<ShaderCompiler> CreateShaderCompiler()` | Create a Slang shader compiler |

### Usage

```cpp
InstanceCreateInfo info;
info.type = BackendType::Vulkan;
info.applicationName = "MyApp";
info.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
auto instance = Instance::Create(info);
```

---

## Adapter

Represents a physical GPU. Used to query device properties, supported features, and create logical devices.

### AdapterProperties

```cpp
struct AdapterProperties {
    uint32_t deviceID;
    uint32_t vendorID;
    std::string name;
    AdapterType type;            // Discrete, Integrated, Software, Unknown
    uint64_t dedicatedVideoMemory;
};
```

### DeviceFeature

```cpp
enum class DeviceFeature : uint32_t {
    MultiViewport, DrawIndirectCount, GeometryShader, TessellationShader,
    MultiDrawIndirect, FillModeNonSolid, WideLines, SamplerAnisotropy,
    TextureCompressionBC, TextureCompressionASTC, BindlessDescriptors,
    BufferDeviceAddress, MeshShader, TaskShader, RayTracingPipeline,
    RayTracingQuery, AccelerationStructure, VariableRateShading,
    ConditionalRendering, ShaderFloat64, ShaderInt16, ShaderInt64,
    SubgroupOperations, RayTracing, ShaderFloat16, IndependentBlending,
    PipelineStatistics
};
```

### DeviceLimits

Hardware capability limits queried from the adapter:

| Field | Default | Description |
|---|---|---|
| `maxTextureSize2D` | 16384 | Max 2D texture dimension |
| `maxTextureSize3D` | 2048 | Max 3D texture dimension |
| `maxColorAttachments` | 8 | Max simultaneous render targets |
| `maxViewports` | 16 | Max viewports |
| `maxComputeWorkGroupSizeX/Y/Z` | 1024/1024/64 | Max compute work group size |
| `maxComputeSharedMemorySize` | 32768 | Max shared memory in bytes |
| `maxBoundDescriptorSets` | 8 | Max descriptor sets per pipeline |
| `maxPushConstantsSize` | 128 | Max push constant size in bytes |
| `maxBufferSize` | 256MB | Max single buffer size |
| `supportsAsyncCompute` | false | Async compute queue support |
| `supportsTransferQueue` | false | Dedicated transfer queue support |

### Adapter Methods

| Method | Description |
|---|---|
| `AdapterProperties GetProperties() const` | Get GPU name, type, memory info |
| `AdapterType GetAdapterType() const` | Discrete / Integrated / Software |
| `bool IsFeatureSupported(DeviceFeature) const` | Query feature support |
| `DeviceLimits QueryLimits() const` | Query hardware limits |
| `Ref<Device> CreateDevice(const DeviceCreateInfo&)` | Create a logical device |
| `uint32_t FindQueueFamilyIndex(QueueType) const` | Find queue family for a given type |

### Usage

```cpp
auto adapters = instance->EnumerateAdapters();
for (auto& adapter : adapters) {
    auto props = adapter->GetProperties();
    std::cout << props.name << " (" << props.dedicatedVideoMemory / (1024*1024) << " MB)" << std::endl;
    if (adapter->IsFeatureSupported(DeviceFeature::RayTracingPipeline))
        std::cout << "  Supports ray tracing!" << std::endl;
}
```

---

## Device

Logical device for creating GPU resources. Created from an Adapter.

### DeviceCreateInfo

```cpp
struct DeviceCreateInfo {
    std::vector<DeviceFeature> EnabledFeatures;
    std::vector<QueueRequest> QueueRequests;
    Ref<Surface> CompatibleSurface = nullptr;   // Required for presentation
    void* Next = nullptr;                        // Backend-specific extension chain
};

struct QueueRequest {
    QueueType Type;          // Graphics, Compute, Transfer, Present
    uint32_t Count = 1;
    float Priority = 1.0f;
};
```

### Device Methods

| Method | Description |
|---|---|
| `Ref<Queue> GetQueue(QueueType, uint32_t index)` | Get a command queue |
| `Ref<Swapchain> CreateSwapchain(const SwapchainCreateInfo&)` | Create a swapchain |
| `Ref<CommandBufferEncoder> CreateCommandBufferEncoder(CommandBufferType)` | Create a command buffer |
| `Ref<Texture> CreateTexture(const TextureCreateInfo&)` | Create a texture |
| `Ref<Buffer> CreateBuffer(const BufferCreateInfo&)` | Create a buffer |
| `Ref<Sampler> CreateSampler(const SamplerCreateInfo&)` | Create a texture sampler |
| `Ref<DescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo&)` | Create descriptor set layout |
| `Ref<DescriptorPool> CreateDescriptorPool(const DescriptorPoolCreateInfo&)` | Create descriptor pool |
| `Ref<ShaderModule> CreateShaderModule(const ShaderBlob&, const ShaderCreateInfo&)` | Create a shader module |
| `Ref<PipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo&)` | Create pipeline layout |
| `Ref<PipelineCache> CreatePipelineCache(std::span<const uint8_t>)` | Create/restore pipeline cache |
| `Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo&)` | Create graphics pipeline |
| `Ref<ComputePipeline> CreateComputePipeline(const ComputePipelineCreateInfo&)` | Create compute pipeline |
| `Ref<Synchronization> CreateSynchronization(uint32_t maxFramesInFlight)` | Create frame synchronization |
| `Ref<QueryPool> CreateQueryPool(const QueryPoolCreateInfo&)` | Create query pool (timestamps, occlusion) |
| `Ref<TimelineSemaphore> CreateTimelineSemaphore(uint64_t)` | Create timeline semaphore |
| `Ref<AccelerationStructure> CreateAccelerationStructure(...)` | Create ray tracing acceleration structure |
| `Ref<RayTracingPipeline> CreateRayTracingPipeline(...)` | Create ray tracing pipeline |

### Usage

```cpp
DeviceCreateInfo dci;
dci.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
dci.CompatibleSurface = surface;
dci.EnabledFeatures = {DeviceFeature::SamplerAnisotropy};
auto device = adapter->CreateDevice(dci);
```

---

## Queue

Command submission queue. Obtained from Device.

### QueueType

```cpp
enum class QueueType { Graphics, Compute, Transfer, Present };
```

### Queue Methods

| Method | Description |
|---|---|
| `QueueType GetType() const` | Get queue type |
| `uint32_t GetIndex() const` | Get queue index |
| `void Submit(cmd, sync, frameIndex)` | Submit with frame synchronization |
| `void Submit(cmds, sync, frameIndex)` | Batch submit multiple command buffers |
| `void Submit(cmd)` | Submit without synchronization (blocking) |
| `void WaitIdle()` | Wait for all submitted work to complete |

---

## Swapchain

Manages presentation images for rendering to a window surface.

### SwapchainCreateInfo

```cpp
struct SwapchainCreateInfo {
    Extent2D Extent;
    Format Format = Format::BGRA8_UNORM;
    ColorSpace ColorSpace = ColorSpace::SRGB_NONLINEAR;
    PresentMode PresentMode = PresentMode::Mailbox;
    uint32_t MinImageCount = 3;
    SurfaceTransform PreTransform;
    CompositeAlpha CompositeAlpha = CompositeAlpha::Opaque;
    SwapchainUsageFlags Usage = SwapchainUsageFlags::ColorAttachment;
    bool Clipped = true;
    Ref<Surface> CompatibleSurface = nullptr;
    uint32_t ImageArrayLayers = 1;
};
```

### PresentMode

| Mode | Description |
|---|---|
| `Immediate` | No vsync, lowest latency, may tear |
| `Mailbox` | Triple buffering, low latency, no tearing |
| `Fifo` | Vsync, guaranteed supported |
| `FifoRelaxed` | Vsync with late frame allowance |

### Swapchain Methods

| Method | Description |
|---|---|
| `Result AcquireNextImage(sync, frameIndex, &imageIndex)` | Acquire next presentable image |
| `Result Present(queue, sync, frameIndex)` | Present rendered image |
| `uint32_t GetImageCount() const` | Number of swapchain images |
| `Ref<Texture> GetBackBuffer(uint32_t index) const` | Get swapchain image as texture |
| `Extent2D GetExtent() const` | Get swapchain dimensions |
| `Format GetFormat() const` | Get surface format |

### Usage (Builder Pattern)

```cpp
auto swapchain = device->CreateSwapchain(
    SwapchainBuilder()
        .SetExtent(1280, 720)
        .SetFormat(Format::BGRA8_UNORM)
        .SetPresentMode(PresentMode::Fifo)
        .SetMinImageCount(3)
        .SetSurface(surface)
        .Build());
```

---

## Buffer

GPU memory buffer for vertex data, index data, uniforms, storage, etc.

### BufferUsageFlags

```cpp
enum class BufferUsageFlags : uint32_t {
    TransferSrc, TransferDst,
    UniformBuffer, StorageBuffer, StorageBufferReadOnly, StorageBufferReadWrite,
    IndexBuffer, VertexBuffer, IndirectBuffer,
    ShaderDeviceAddress, AccelerationStructure
};
```

### BufferMemoryUsage

| Mode | Description |
|---|---|
| `GpuOnly` | Device-local, fastest GPU access |
| `CpuToGpu` | Host-visible, for upload (staging, uniforms) |
| `GpuToCpu` | Host-visible, for readback |
| `CpuOnly` | System memory |

### Buffer Methods

| Method | Description |
|---|---|
| `uint64_t GetSize() const` | Buffer size in bytes |
| `void* Map()` | Map buffer for CPU access |
| `void Unmap()` | Unmap buffer |
| `void Flush(offset, size)` | Flush mapped range to GPU |
| `void Write<T>(data, offset)` | Template helper: map + memcpy |
| `uint64_t GetDeviceAddress() const` | Get BDA (for ray tracing, indirect) |

### Usage (Builder Pattern)

```cpp
// Vertex buffer
auto vbo = device->CreateBuffer(
    BufferBuilder()
        .SetSize(vertices.size() * sizeof(Vertex))
        .SetUsage(BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("VertexBuffer")
        .Build());
vbo->Write(vertices);
vbo->Flush();

// Uniform buffer
auto ubo = device->CreateBuffer(
    BufferBuilder()
        .SetSize(sizeof(UniformData))
        .SetUsage(BufferUsageFlags::UniformBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .Build());
```

---

## Texture

GPU image resources for sampling, render targets, and storage.

### TextureType

```cpp
enum class TextureType {
    Texture1D, Texture2D, Texture3D, TextureCube,
    Texture1DArray, Texture2DArray, TextureCubeArray
};
```

### TextureUsageFlags

```cpp
enum class TextureUsageFlags : uint32_t {
    TransferSrc, TransferDst,
    Sampled,                    // Can be read in shaders
    Storage,                    // Can be written in compute shaders
    ColorAttachment,            // Render target
    DepthStencilAttachment,     // Depth/stencil target
    TransientAttachment,        // Lazily allocated
    InputAttachment             // Subpass input
};
```

### Texture Methods

| Method | Description |
|---|---|
| `uint32_t GetWidth/Height/Depth() const` | Texture dimensions |
| `uint32_t GetMipLevels() const` | Number of mip levels |
| `uint32_t GetArrayLayers() const` | Number of array layers |
| `Format GetFormat() const` | Pixel format |
| `Ref<CacaoTextureView> CreateView(desc)` | Create a custom texture view |
| `Ref<CacaoTextureView> GetDefaultView()` | Get the default view |
| `void CreateDefaultViewIfNeeded()` | Ensure default view exists |
| `bool IsDepthStencil() const` | Check if depth/stencil format |

### Texture Views

```cpp
struct TextureViewDesc {
    TextureType ViewType = TextureType::Texture2D;
    Format FormatOverride = Format::UNDEFINED;  // UNDEFINED = use texture format
    uint32_t BaseMipLevel = 0;
    uint32_t MipLevelCount = 1;
    uint32_t BaseArrayLayer = 0;
    uint32_t ArrayLayerCount = 1;
    AspectMask Aspect = AspectMask::Color;
};
```

### Usage (Builder Pattern)

```cpp
auto texture = device->CreateTexture(
    TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(1024, 1024)
        .SetFormat(Format::RGBA8_UNORM)
        .SetMipLevels(10)
        .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
        .SetName("Albedo")
        .Build());

// Depth buffer
auto depthBuffer = device->CreateTexture(
    TextureBuilder()
        .SetSize(1280, 720)
        .SetFormat(Format::D32F)
        .SetUsage(TextureUsageFlags::DepthStencilAttachment)
        .Build());
```

---

## Sampler

Texture sampling configuration.

### SamplerCreateInfo

```cpp
struct SamplerCreateInfo {
    Filter MagFilter = Filter::Linear;
    Filter MinFilter = Filter::Linear;
    SamplerMipmapMode MipmapMode = SamplerMipmapMode::Linear;
    SamplerAddressMode AddressModeU/V/W = SamplerAddressMode::Repeat;
    float MipLodBias = 0.0f;
    float MinLod = 0.0f;
    float MaxLod = 1000.0f;
    bool AnisotropyEnable = true;
    float MaxAnisotropy = 16.0f;
    bool CompareEnable = false;
    CompareOp CompareOp = CompareOp::LessOrEqual;
    BorderColor BorderColor = BorderColor::FloatOpaqueBlack;
    bool UnnormalizedCoordinates = false;
};
```

### Usage (Builder Pattern)

```cpp
auto sampler = device->CreateSampler(
    SamplerBuilder()
        .SetFilter(Filter::Linear, Filter::Linear)
        .SetMipmapMode(SamplerMipmapMode::Linear)
        .SetAddressMode(SamplerAddressMode::Repeat)
        .SetAnisotropy(true, 16.0f)
        .Build());

// Shadow map sampler
auto shadowSampler = device->CreateSampler(
    SamplerBuilder()
        .SetFilter(Filter::Linear, Filter::Linear)
        .SetAddressMode(SamplerAddressMode::ClampToBorder)
        .SetCompare(true, CompareOp::LessOrEqual)
        .SetBorderColor(BorderColor::FloatOpaqueWhite)
        .Build());
```

---

## Pipeline

Encapsulates the full GPU pipeline state.

### GraphicsPipeline

```cpp
struct GraphicsPipelineCreateInfo {
    std::vector<Ref<ShaderModule>> Shaders;
    std::vector<VertexInputBinding> VertexBindings;
    std::vector<VertexInputAttribute> VertexAttributes;
    InputAssemblyState InputAssembly;
    RasterizationState Rasterizer;
    DepthStencilState DepthStencil;
    ColorBlendState ColorBlend;
    MultisampleState Multisample;
    std::vector<Format> ColorAttachmentFormats;
    Format DepthStencilFormat = Format::UNDEFINED;
    Ref<PipelineLayout> Layout;
    Ref<PipelineCache> Cache = nullptr;
};
```

### ComputePipeline

```cpp
struct ComputePipelineCreateInfo {
    Ref<ShaderModule> ComputeShader;
    Ref<PipelineLayout> Layout;
    Ref<PipelineCache> Cache = nullptr;
};
```

### RayTracingPipeline

```cpp
struct RayTracingPipelineCreateInfo {
    std::vector<Ref<ShaderModule>> Shaders;
    uint32_t MaxRecursionDepth = 1;
    Ref<PipelineLayout> Layout;
    Ref<PipelineCache> Cache = nullptr;
};
```

### PipelineCache

Serialize and restore pipeline compilation results to reduce startup time:

```cpp
auto cache = device->CreatePipelineCache();
// ... create pipelines with cache ...

// Save to disk
auto cacheData = cache->GetData();
std::ofstream("pipeline.cache", std::ios::binary)
    .write((char*)cacheData.data(), cacheData.size());

// Restore from disk
std::ifstream ifs("pipeline.cache", std::ios::binary);
std::vector<uint8_t> loadedData(/*...*/);
auto restoredCache = device->CreatePipelineCache(loadedData);
```

### Usage (Builder Pattern)

```cpp
auto pipeline = device->CreateGraphicsPipeline(
    GraphicsPipelineBuilder()
        .SetShaders({vertexShader, fragmentShader})
        .AddVertexBinding(0, sizeof(Vertex), VertexInputRate::Vertex)
        .AddVertexAttribute(0, 0, Format::RGB32_FLOAT, offsetof(Vertex, position))
        .AddVertexAttribute(1, 0, Format::RG32_FLOAT, offsetof(Vertex, texcoord))
        .SetTopology(PrimitiveTopology::TriangleList)
        .SetCullMode(CullMode::Back)
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetDepthTest(true, true, CompareOp::Less)
        .AddColorAttachmentDefault(false)
        .AddColorFormat(Format::BGRA8_UNORM)
        .SetDepthStencilFormat(Format::D32F)
        .SetLayout(pipelineLayout)
        .Build());
```

---

## PipelineLayout

Describes the resource binding layout for a pipeline.

### PipelineLayoutCreateInfo

```cpp
struct PipelineLayoutCreateInfo {
    std::vector<Ref<DescriptorSetLayout>> SetLayouts;
    std::vector<PushConstantRange> PushConstantRanges;
};

struct PushConstantRange {
    ShaderStage StageFlags;
    uint32_t Offset;
    uint32_t Size;
};
```

### Usage (Builder Pattern)

```cpp
auto layout = device->CreatePipelineLayout(
    PipelineLayoutBuilder()
        .AddSetLayout(descriptorSetLayout)
        .AddPushConstant(ShaderStage::Vertex | ShaderStage::Fragment, 0, sizeof(PushData))
        .Build());
```

---

## DescriptorSetLayout & DescriptorSet

Describes how shader resources (buffers, textures, samplers) are bound.

### DescriptorType

```cpp
enum class DescriptorType {
    Sampler,                 // Standalone sampler
    CombinedImageSampler,    // Texture + sampler combined
    SampledImage,            // Texture for sampling
    StorageImage,            // Read/write texture
    UniformBuffer,           // Constant buffer
    StorageBuffer,           // Read/write buffer (SSBO)
    UniformBufferDynamic,    // UBO with dynamic offset
    StorageBufferDynamic,    // SSBO with dynamic offset
    InputAttachment,         // Subpass input
    AccelerationStructure    // Ray tracing TLAS
};
```

### DescriptorSetLayout Creation

```cpp
auto layout = device->CreateDescriptorSetLayout(
    DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex)
        .AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment)
        .AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment)
        .Build());
```

### DescriptorPool & DescriptorSet Allocation

```cpp
auto pool = device->CreateDescriptorPool(
    DescriptorPoolBuilder()
        .SetMaxSets(10)
        .AddPoolSize(DescriptorType::UniformBuffer, 10)
        .AddPoolSize(DescriptorType::SampledImage, 10)
        .AddPoolSize(DescriptorType::Sampler, 10)
        .Build());

auto descriptorSet = pool->AllocateDescriptorSet(layout);
```

### Writing Descriptors

```cpp
// Write a buffer
descriptorSet->WriteBuffer({
    .Binding = 0,
    .Buffer = uniformBuffer,
    .Offset = 0,
    .Size = sizeof(UniformData),
    .Type = DescriptorType::UniformBuffer
});

// Write a texture
descriptorSet->WriteTexture({
    .Binding = 1,
    .TextureView = texture->GetDefaultView(),
    .Layout = ResourceState::ShaderRead,
    .Type = DescriptorType::SampledImage
});

// Write a sampler
descriptorSet->WriteSampler({.Binding = 2, .Sampler = sampler});

// Commit all writes
descriptorSet->Update();
```

### Batch Writing

```cpp
descriptorSet->WriteBuffers({
    .Binding = 0,
    .Buffers = {buf1, buf2, buf3},
    .Offsets = {0, 0, 0},
    .Sizes = {size1, size2, size3},
    .Type = DescriptorType::StorageBuffer
});
```

---

## CommandBufferEncoder

Records GPU commands for submission to a queue.

### Render Pass Commands

```cpp
cmd->Begin();

// Transition swapchain image
cmd->TransitionImage(backBuffer, ImageTransition::UndefinedToColorAttachment);

// Begin dynamic rendering
RenderingInfo ri;
ri.RenderArea = {0, 0, width, height};
ri.ColorAttachments = {{
    .Texture = backBuffer,
    .LoadOp = AttachmentLoadOp::Clear,
    .StoreOp = AttachmentStoreOp::Store,
    .ClearValue = ClearValue::ColorFloat(0.1f, 0.1f, 0.1f, 1.0f)
}};
cmd->BeginRendering(ri);

// Set viewport and scissor
cmd->SetViewport({0, 0, (float)width, (float)height, 0.0f, 1.0f});
cmd->SetScissor({0, 0, width, height});

// Bind pipeline and resources
cmd->BindGraphicsPipeline(pipeline);
cmd->BindDescriptorSets(pipeline, 0, {descriptorSet});
cmd->PushConstants(pipeline, ShaderStage::Vertex, 0, sizeof(pushData), &pushData);

// Bind vertex/index buffers
cmd->BindVertexBuffer(0, vertexBuffer);
cmd->BindIndexBuffer(indexBuffer, 0, IndexType::UInt32);

// Draw
cmd->DrawIndexed(indexCount, 1, 0, 0, 0);

cmd->EndRendering();
cmd->TransitionImage(backBuffer, ImageTransition::ColorAttachmentToPresent);
cmd->End();
```

### Draw Commands

| Method | Description |
|---|---|
| `Draw(vertexCount, instanceCount, firstVertex, firstInstance)` | Non-indexed draw |
| `DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)` | Indexed draw |
| `DrawIndirect(argBuffer, offset, drawCount, stride)` | GPU-driven draw |
| `DrawIndexedIndirect(argBuffer, offset, drawCount, stride)` | GPU-driven indexed draw |
| `DrawIndirectCount(argBuffer, offset, countBuffer, countOffset, maxDrawCount, stride)` | GPU-driven with GPU count |
| `DispatchMesh(groupX, groupY, groupZ)` | Mesh shader dispatch |

### Compute Commands

```cpp
cmd->BindComputePipeline(computePipeline);
cmd->BindComputeDescriptorSets(computePipeline, 0, {computeDescSet});
cmd->Dispatch(groupCountX, groupCountY, groupCountZ);
cmd->DispatchIndirect(argBuffer, offset);
```

### Transfer Commands

| Method | Description |
|---|---|
| `CopyBuffer(src, dst, srcOffset, dstOffset, size)` | Buffer-to-buffer copy |
| `CopyBufferToImage(srcBuffer, dstTexture, layout, regions)` | Upload texture data |
| `ResolveTexture(src, dst, srcSub, dstSub)` | MSAA resolve |

### Barrier Commands

```cpp
// High-level image transition
cmd->TransitionImage(texture, ImageTransition::UndefinedToShaderRead);

// High-level buffer transition
cmd->TransitionBuffer(buffer, BufferTransition::HostWriteToVertexRead);

// Quick memory barrier
cmd->MemoryBarrierFast(MemoryTransition::ComputeWriteToGraphicsRead);

// Full pipeline barrier
cmd->PipelineBarrier(
    SyncScope::ComputeStage,
    SyncScope::FragmentStage,
    {},  // global barriers
    {BufferBarrier{buffer, ResourceState::UnorderedAccess, ResourceState::ShaderRead}},
    {}   // texture barriers
);
```

### Ray Tracing Commands

```cpp
cmd->BindRayTracingPipeline(rtPipeline);
cmd->BindRayTracingDescriptorSets(rtPipeline, 0, {rtDescSet});
cmd->BuildAccelerationStructure(accelStruct);
cmd->TraceRays(sbt, width, height, 1);
```

### Debug Markers

```cpp
cmd->BeginDebugLabel("Shadow Pass", 1.0f, 0.5f, 0.0f);
// ... draw commands ...
cmd->EndDebugLabel();
cmd->InsertDebugLabel("Checkpoint");
```

### Query Commands

```cpp
cmd->ResetQueryPool(queryPool, 0, queryCount);
cmd->WriteTimestamp(queryPool, 0);
// ... render work ...
cmd->WriteTimestamp(queryPool, 1);
```

---

## Synchronization

Frame-level synchronization for multi-buffered rendering.

### Methods

| Method | Description |
|---|---|
| `void WaitForFrame(uint32_t frameIndex)` | Wait for a frame's GPU work to complete |
| `void ResetFrameFence(uint32_t frameIndex)` | Reset frame fence for reuse |
| `uint32_t AcquireNextImageIndex(swapchain, frameIndex)` | Acquire next swapchain image |
| `uint32_t GetMaxFramesInFlight()` | Max concurrent frames |
| `void WaitIdle()` | Wait for all GPU work to complete |

### TimelineSemaphore

For fine-grained GPU-GPU and CPU-GPU synchronization:

```cpp
auto timeline = device->CreateTimelineSemaphore(0);
timeline->Signal(42);
timeline->Wait(42);
uint64_t current = timeline->GetValue();
```

### Render Loop Pattern

```cpp
uint32_t frame = 0;
while (running) {
    sync->WaitForFrame(frame);

    int imageIndex;
    swapchain->AcquireNextImage(sync, frame, imageIndex);
    sync->ResetFrameFence(frame);

    // Record and submit commands...
    queue->Submit(cmd, sync, frame);
    swapchain->Present(queue, sync, frame);

    frame = (frame + 1) % sync->GetMaxFramesInFlight();
}
sync->WaitIdle();
```

---

## QueryPool

GPU-side timestamp and occlusion queries.

### QueryType

```cpp
enum class QueryType { Occlusion, Timestamp, PipelineStatistics };
```

### Methods

| Method | Description |
|---|---|
| `void Reset(firstQuery, count)` | Reset queries for reuse |
| `bool GetResults(first, count, &results, wait)` | Read query results |
| `QueryType GetType() const` | Get pool type |

### GPU Timestamp Example

```cpp
auto pool = device->CreateQueryPool({QueryType::Timestamp, 64});

cmd->ResetQueryPool(pool, 0, 2);
cmd->WriteTimestamp(pool, 0);
// ... GPU work ...
cmd->WriteTimestamp(pool, 1);

// After submit + wait
std::vector<uint64_t> results;
pool->GetResults(0, 2, results, true);
double deltaMs = (results[1] - results[0]) * timestampPeriod * 1e-6;
```

---

## Surface

Window surface for presentation. Platform-specific.

### NativeWindowHandle

```cpp
struct NativeWindowHandle {
#if defined(_WIN32)
    void* hWnd = nullptr;
    void* hInst = nullptr;
#elif defined(__APPLE__)
    void* metalLayer = nullptr;
#elif defined(__linux__)
    // Wayland, XCB, or Xlib handles
#elif defined(__ANDROID__)
    void* aNativeWindow = nullptr;
#endif
};
```

### Surface Methods

| Method | Description |
|---|---|
| `SurfaceCapabilities GetCapabilities(adapter)` | Query surface limits and formats |
| `std::vector<SurfaceFormat> GetSupportedFormats(adapter)` | List supported formats |
| `std::vector<PresentMode> GetSupportedPresentModes(adapter)` | List supported present modes |
| `Ref<Queue> GetPresentQueue(device)` | Get the present-capable queue |

---

## EasyContext

High-level helper that wraps all initialization into a single object. Ideal for prototyping.

### Methods

| Method | Description |
|---|---|
| `static Ref<EasyContext> Create(const EasyConfig&)` | One-call initialization |
| `Ref<CommandBufferEncoder> BeginFrame()` | Acquire image + create command buffer |
| `void EndFrame(cmd)` | Submit + present |
| `Ref<ShaderModule> LoadShader(path, entry, stage)` | Compile shader from file |
| `Ref<Buffer> CreateVertexBuffer(size, data)` | Quick vertex buffer |
| `Ref<Buffer> CreateIndexBuffer(size, data)` | Quick index buffer |
| `Ref<Buffer> CreateStorageBuffer(size, data)` | Quick SSBO |
| `Ref<Buffer> CreateUniformBuffer(size)` | Quick UBO |
| `QuickPipeline CreateQuickPipeline(shaderPath, ...)` | Pipeline + layout + pool in one call |
| `void ClearAndBeginRendering(cmd, r, g, b, a)` | Begin render pass with clear |
| `void EndRendering(cmd)` | End render pass + transition to present |

---

## Builder Summary

All resource creation supports builder pattern for clean, chainable configuration:

| Builder | Creates |
|---|---|
| `BufferBuilder` | `BufferCreateInfo` |
| `TextureBuilder` | `TextureCreateInfo` |
| `SamplerBuilder` | `SamplerCreateInfo` |
| `DescriptorSetLayoutBuilder` | `DescriptorSetLayoutCreateInfo` |
| `DescriptorPoolBuilder` | `DescriptorPoolCreateInfo` |
| `PipelineLayoutBuilder` | `PipelineLayoutCreateInfo` |
| `GraphicsPipelineBuilder` | `GraphicsPipelineCreateInfo` |
| `ComputePipelineBuilder` | `ComputePipelineCreateInfo` |
| `SwapchainBuilder` | `SwapchainCreateInfo` |

Each builder returns `const XxxCreateInfo&` from `.Build()` and also supports implicit conversion.
