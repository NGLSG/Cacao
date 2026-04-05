# API 参考

## Instance (实例)

RHI 入口点，创建后端特定的实现。

```cpp
InstanceCreateInfo info;
info.type = BackendType::DirectX12;  // Vulkan, DirectX11, OpenGL 等
info.applicationName = "MyApp";
info.enabledFeatures = {InstanceFeature::ValidationLayer};
auto instance = Instance::Create(info);
```

**方法:**
- `EnumerateAdapters()` — 列出可用 GPU
- `CreateSurface(NativeWindowHandle)` — 创建窗口表面
- `CreateShaderCompiler()` — 创建 Slang 着色器编译器

## Adapter (适配器)

代表物理 GPU。

```cpp
auto adapters = instance->EnumerateAdapters();
auto props = adapters[0]->GetProperties();  // name, vendorID, memory
auto device = adapters[0]->CreateDevice(deviceCreateInfo);
```

## Device (设备)

逻辑设备，用于创建资源。

```cpp
auto buffer = device->CreateBuffer(BufferBuilder().SetSize(1024).SetUsage(BufferUsageFlags::StorageBuffer).Build());
auto texture = device->CreateTexture(TextureBuilder().SetSize(256, 256).SetFormat(Format::RGBA8_UNORM).Build());
auto sampler = device->CreateSampler(SamplerBuilder().SetFilter(Filter::Linear, Filter::Linear).Build());
auto pipeline = device->CreateGraphicsPipeline(pipelineInfo);
auto cmd = device->CreateCommandBufferEncoder();
auto queue = device->GetQueue(QueueType::Graphics, 0);
```

## Buffer (缓冲区)

GPU 内存缓冲区，用于顶点、索引、Uniform、存储等。

```cpp
auto buf = device->CreateBuffer(
    BufferBuilder()
    .SetSize(sizeof(MyData) * count)
    .SetUsage(BufferUsageFlags::StorageBuffer)
    .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
    .Build());

void* ptr = buf->Map();       // 映射到 CPU 内存
memcpy(ptr, data, size);      // 写入数据
buf->Flush();                  // 同步到 GPU
```

**Usage 标志:** `VertexBuffer`, `IndexBuffer`, `UniformBuffer`, `StorageBuffer`, `TransferSrc`, `TransferDst`

## Texture (纹理)

GPU 图像资源。

```cpp
auto tex = device->CreateTexture(
    TextureBuilder()
    .SetType(TextureType::Texture2D)
    .SetSize(width, height)
    .SetFormat(Format::RGBA8_UNORM)
    .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
    .Build());

tex->CreateDefaultViewIfNeeded();
auto view = tex->GetDefaultView();
```

## Swapchain (交换链)

管理窗口表面的呈现。

```cpp
auto swapchain = device->CreateSwapchain(
    SwapchainBuilder()
    .SetExtent(width, height)
    .SetFormat(Format::BGRA8_UNORM)
    .SetPresentMode(PresentMode::Mailbox)
    .SetMinImageCount(3)
    .SetSurface(surface)
    .Build());

int imageIndex;
swapchain->AcquireNextImage(sync, frameIdx, imageIndex);  // 获取下一帧
auto backBuffer = swapchain->GetBackBuffer(imageIndex);     // 获取后缓冲
swapchain->Present(queue, sync, frameIdx);                  // 呈现
```

## DescriptorSet (描述符集)

将资源（缓冲区、纹理、采样器）绑定到着色器插槽。

```cpp
// 布局定义
auto layout = device->CreateDescriptorSetLayout(
    DescriptorSetLayoutBuilder()
    .AddBinding(0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex)     // SSBO
    .AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment)    // 纹理
    .AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment)         // 采样器
    .Build());

// 池和集
auto pool = device->CreateDescriptorPool(...);
auto descriptorSet = pool->AllocateDescriptorSet(layout);

// 写入资源
descriptorSet->WriteBuffer({0, myBuffer, 0, sizeof(MyStruct), bufferSize, DescriptorType::StorageBuffer});
descriptorSet->WriteTexture({1, textureView, ResourceState::ShaderRead, DescriptorType::SampledImage});
descriptorSet->WriteSampler({2, mySampler});
descriptorSet->Update();
```

## Pipeline (管线)

图形或计算管线状态。

```cpp
auto pipeline = device->CreateGraphicsPipeline(
    GraphicsPipelineBuilder()
    .SetShaders({vertexShader, fragmentShader})
    .SetTopology(PrimitiveTopology::TriangleList)
    .SetCullMode(CullMode::Back)
    .SetDepthTest(true, true)
    .AddColorAttachmentDefault(true)  // Alpha 混合
    .AddColorFormat(Format::BGRA8_UNORM)
    .SetLayout(pipelineLayout)
    .Build());
```

## CommandBufferEncoder (命令编码器)

录制 GPU 命令，延迟执行。

```cpp
auto cmd = device->CreateCommandBufferEncoder();
cmd->Reset();
cmd->Begin({true});

cmd->BeginRendering(renderInfo);              // 开始渲染
cmd->SetViewport({0, 0, w, h, 0, 1});        // 设置视口
cmd->SetScissor({0, 0, w, h});               // 设置裁剪
cmd->BindGraphicsPipeline(pipeline);          // 绑定管线
cmd->BindDescriptorSets(pipeline, 0, {set});  // 绑定描述符
cmd->Draw(vertexCount, instanceCount, 0, 0);  // 绘制
cmd->EndRendering();                          // 结束渲染

cmd->End();
queue->Submit(cmd, sync, frameIndex);         // 提交到 GPU
```

## Synchronization (同步)

帧级别的 GPU 同步。

```cpp
auto sync = device->CreateSynchronization(swapchain->GetImageCount());

// 每帧:
sync->WaitForFrame(frameIndex);   // CPU 等待 GPU 完成
// ... 录制和提交 ...
sync->WaitIdle();                  // 等待所有帧完成
```

## ShaderCompiler (着色器编译器)

将 Slang 着色器编译为后端原生格式。

```cpp
auto compiler = instance->CreateShaderCompiler();
compiler->SetCacheDirectory("shader_cache");

ShaderCreateInfo sci;
sci.SourcePath = "shader.slang";
sci.EntryPoint = "mainVS";
sci.Stage = ShaderStage::Vertex;
auto vs = compiler->CompileOrLoad(device, sci);
```

着色器缓存自动管理 — 编译结果保存到磁盘，后续运行直接加载。

## 屏障与资源转换

### 保证语义（所有后端）
- `TransitionImage()` / `TransitionBuffer()`: 记录资源状态转换。Vulkan/DX12 插入管线屏障；DX11/GL 为空操作。
- `MemoryBarrierFast()`: 轻量级屏障。Vulkan 用 VkMemoryBarrier，GL 用 glMemoryBarrier，DX11/DX12 为空。

### 可选语义（仅现代后端）
- `PipelineBarrier()` 细粒度阶段/访问掩码：仅 Vulkan 提供完整语义。
- 队列所有权转移：仅 Vulkan 支持。

### 最佳实践
始终在改变资源用途前调用 `TransitionImage()`，现代后端需要它，旧后端安全忽略。

## 能力查询系统

通过 `Adapter::QueryLimits()` 查询硬件能力：

```cpp
auto limits = adapter->QueryLimits();
if (limits.supportsAsyncCompute)
    // 使用独立计算队列
if (limits.supportsStorageBufferWriteInGraphics)
    // 在像素着色器中使用 RWStructuredBuffer
```

### 关键能力对照
| 能力 | VK | DX12 | DX11 | GL |
|-----|-----|------|------|----|
| 异步计算 | 是* | 是 | 否 | 否 |
| 传输队列 | 是* | 是 | 否 | 否 |
| 图形阶段存储缓冲区写入 | 是 | 是 | 否 | 是 |
| 管线缓存序列化 | 是 | 是 | 否 | 否 |

\* 取决于 GPU 硬件

## 性能预期

| 操作 | VK/DX12 | DX11 | GL |
|-----|---------|------|----|
| 命令缓冲区录制 | 原生 GPU 命令列表 | 立即执行 | Lambda 捕获 + 延迟回放 |
| 资源屏障 | GPU 端管线同步 | 空操作 | 空操作/glMemoryBarrier |
| 缓冲区映射/刷新 | 持久映射或 staging | Dynamic/Staging 缓冲区 | CPU 影子 + glBufferSubData |
| 描述符绑定 | DescriptorSet + 根签名 | 槽位绑定 | UBO/SSBO/纹理单元绑定 |

## 原生访问（逃生口）

当 RHI 抽象不够时，可直接访问底层 API：

```cpp
// 方法1: ExecuteNative - 内联执行原生命令
cmd->ExecuteNative([](void* nativeHandle) {
    auto* vkCmd = static_cast<VkCommandBuffer>(nativeHandle);
    // 直接调用 Vulkan API
});

// 方法2: GetNativeHandle - 获取原生句柄
void* handle = cmd->GetNativeHandle();

// 方法3: 后端特定类型转换
auto* vkQueue = dynamic_cast<VKQueue*>(queue.get());
if (vkQueue) {
    vk::Queue nativeQueue = vkQueue->GetHandle();
}
```

确保 RHI 永远不会阻止访问底层 GPU API。

## 校验系统

Cacao 在管线和描述符创建时执行硬性校验：

```cpp
// 校验失败时抛出 std::runtime_error：
auto pipeline = device->CreateGraphicsPipeline(info);  // 自动校验
auto layout = device->CreateDescriptorSetLayout(info);  // 校验 StorageBuffer 能力

// 着色器-布局不匹配检测：
std::string error;
if (!ShaderCompiler::ValidateShaderAgainstLayout(reflection, layoutInfo, error))
    std::cerr << "不匹配: " << error << std::endl;
```

### 校验内容
| 检查项 | 抛出异常条件 |
|--------|------------|
| 颜色附件数量 | 超过 `QueryLimits().maxColorAttachments` |
| MSAA 采样数 | 超过 `QueryLimits().maxMSAASamples` |
| 着色器模块 | 任何着色器为空或列表为空 |
| 管线布局 | Layout 为空 |
| 片段着色器 StorageBuffer | 后端不支持 `supportsStorageBufferWriteInGraphics` |
| 着色器绑定类型 | 着色器期望的 DescriptorType 与布局不一致 |
| SimultaneousUse (GL) | Tier 2 后端不支持可重用命令缓冲区 |
| BindVertexBuffer 用途 | Buffer 缺少 `VertexBuffer` usage flag |
| BindIndexBuffer 用途 | Buffer 缺少 `IndexBuffer` usage flag |
| CopyBufferToImage 转换 | 目标纹理未在复制前转换 |
| BeginRendering 转换 | 颜色附件未在渲染前转换 |

### 能力驱动的代码

编写一套代码，根据后端能力自动适配：

```cpp
auto limits = adapter->QueryLimits();

DescriptorSetLayoutCreateInfo layout;
if (limits.supportsStorageBufferWriteInGraphics) {
    // Tier 1: 片段着色器中使用 SSBO
    layout.Bindings.push_back({0, DescriptorType::StorageBuffer, 1, ShaderStage::Fragment});
} else {
    // Tier 2: 回退到仅计算着色器使用 SSBO
    layout.Bindings.push_back({0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute});
}

if (limits.supportsAsyncCompute) {
    auto computeQueue = device->GetQueue(QueueType::Compute, 0);
    // 异步计算
} else {
    auto queue = device->GetQueue(QueueType::Graphics, 0);
    // 回退: 在图形队列执行计算
}
```
