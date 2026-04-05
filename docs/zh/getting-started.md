# Cacao RHI 入门指南

## 简介

Cacao 是一个跨后端的图形渲染抽象层（RHI），提供统一的 C++ API 来操作不同的 GPU 后端：

- **DirectX 12** — Windows 平台主力后端
- **Vulkan** — 跨平台后端
- **DirectX 11** / **OpenGL** — 兼容后端

Cacao 支持光栅化管线、计算管线和光线追踪管线，使用 Slang 作为着色器语言。

## 环境要求

| 依赖 | 最低版本 |
|------|---------|
| Windows | 10/11 |
| MSVC | Visual Studio 2022 |
| CMake | 3.20+ |
| Vulkan SDK | 1.3+（使用 Vulkan 后端时需要）|

## 构建方法

```bash
# 配置（推荐使用 Ninja）
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 编译所有目标
cmake --build cmake-build-debug

# 编译单个 demo
cmake --build cmake-build-debug --target HelloTriangle
```

## 核心概念

### Instance

Cacao 的入口点，负责后端初始化和全局状态管理。

```cpp
InstanceCreateInfo info;
info.type = BackendType::DirectX12;
info.applicationName = "MyApp";
info.enabledFeatures = {InstanceFeature::ValidationLayer, InstanceFeature::Surface};
auto instance = Instance::Create(info);
```

### Adapter

GPU 适配器，代表一块物理显卡。通过 `Instance::EnumerateAdapters()` 获取。

```cpp
auto adapter = instance->EnumerateAdapters().front();
std::cout << "GPU: " << adapter->GetProperties().name << std::endl;
```

### Device

逻辑设备，是创建所有 GPU 资源的工厂。

```cpp
DeviceCreateInfo deviceCI;
deviceCI.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};
deviceCI.CompatibleSurface = surface;
auto device = adapter->CreateDevice(deviceCI);
```

### Surface & Swapchain

Surface 连接窗口系统和图形后端；Swapchain 管理多个后缓冲区，实现无闪烁的帧呈现。

```cpp
auto swapchain = device->CreateSwapchain(
    SwapchainBuilder()
    .SetExtent(surfaceCaps.currentExtent)
    .SetFormat(Format::BGRA8_UNORM)
    .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
    .SetPresentMode(PresentMode::Fifo)  // 垂直同步
    .SetMinImageCount(imageCount)
    .SetSurface(surface)
    .Build());
```

### CommandBufferEncoder

命令编码器，录制 GPU 命令（绘制、计算、拷贝、屏障等）。

```cpp
auto cmd = device->CreateCommandBufferEncoder(CommandBufferType::Primary);
cmd->Begin({true});
// ... 录制命令 ...
cmd->End();
graphicsQueue->Submit(cmd, sync, frame);
```

### GraphicsPipeline

图形管线状态对象，封装着色器、顶点格式、光栅化设置、混合模式等。

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

资源绑定集，将 Buffer、Texture、Sampler 等资源绑定到管线的 shader 槽位。

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

## 渲染一帧的基本流程

```
1. sync->WaitForFrame(frame)        // 等待 GPU 完成旧帧
2. swapchain->AcquireNextImage()     // 获取可用后缓冲区
3. sync->ResetFrameFence(frame)      // 重置 fence
4. cmd->Begin()                      // 开始录制命令
5. TransitionImage → ColorAttachment // 资源状态转换
6. BeginRendering()                  // 开始渲染通道
7. SetViewport / SetScissor          // 设置渲染区域
8. BindPipeline / BindVertexBuffer   // 绑定资源
9. Draw / DrawIndexed                // 绘制
10. EndRendering()                   // 结束渲染通道
11. TransitionImage → Present        // 准备呈现
12. cmd->End()                       // 结束录制
13. queue->Submit(cmd, sync, frame)  // 提交到 GPU
14. swapchain->Present()             // 显示到屏幕
```

## 示例一览

| Demo | 描述 | 关键特性 |
|------|------|---------|
| **HelloTriangle** | 彩色三角形 | 最简光栅化，顶点颜色插值 |
| **Scene3D** | 旋转彩色立方体 | MVP 矩阵，UniformBuffer，索引绘制 |
| **TexturedQuad** | 纹理四边形 | 纹理加载，采样器，SampledImage |
| **ComputeDemo** | 计算管线 | Compute Shader，SSBO |
| **ShadowMap** | 阴影贴图 | 多 Pass 渲染，深度纹理 |
| **CornellBox** | 路径追踪 | 光线追踪管线，加速结构，SBT |

## Shader 编写（Slang）

Cacao 使用 [Slang](https://shader-slang.com/) 作为着色器语言，语法与 HLSL 高度兼容。

### 入口点标记

```hlsl
[shader("vertex")]
VSOutput mainVS(VSInput input) { ... }

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target { ... }
```

### 顶点语义

C++ 端 `AddVertexAttribute` 的 `semanticName` 默认为 `"TEXCOORD"`。如需自定义：

```cpp
// C++ 端指定 POSITION 语义
.AddVertexAttribute(0, 0, Format::RGB32_FLOAT, 0, "POSITION", 0)
```

```hlsl
// Shader 端匹配
float3 position : POSITION;
```

### 资源绑定

```hlsl
// Uniform Buffer (cbuffer)
[[vk::binding(0, 0)]]
cbuffer MVPBuffer : register(b0) { float4x4 mvp; };

// Storage Buffer
[[vk::binding(0, 0)]]
StructuredBuffer<MyStruct> data : register(t0);

// 纹理 + 采样器
[[vk::binding(1, 0)]]
Texture2D myTexture : register(t1);
[[vk::binding(2, 0)]]
SamplerState mySampler : register(s2);
```

### 矩阵约定

Cacao 使用列优先（column-major）矩阵。在 Slang 中用行向量乘矩阵：

```hlsl
output.position = mul(float4(pos, 1.0), mvpMatrix);
```

## 跨后端注意事项

| 项目 | DX12 | Vulkan |
|------|------|--------|
| Swapchain 格式 | `BGRA8_UNORM` 常用 | `BGRA8_UNORM` 或 `RGBA8_UNORM` |
| Shader 字节码 | DXIL | SPIR-V |
| Y 轴方向 | 框架自动统一 | 框架自动统一 |
| 验证层 | D3D12 Debug Layer | Vulkan Validation Layer |
| 后端选择 | `BackendType::DirectX12` | `BackendType::Vulkan` |

Cacao 的设计目标是让应用代码完全后端无关，只需在 `InstanceCreateInfo::type` 中选择后端即可。Shader 用 Slang 编写，框架自动编译为对应后端的字节码。
