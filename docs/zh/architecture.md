# 架构设计

## 抽象层次

```
┌─────────────────────────────┐
│        应用层                │
│    (main.cpp / 引擎)         │
├─────────────────────────────┤
│      Cacao RHI API          │
│  Instance / Device / Queue  │
│  Buffer / Texture / Sampler │
│  DescriptorSet / Pipeline   │
│  CommandBuffer / Swapchain  │
│  Synchronization / Query    │
├──────┬──────┬──────┬────────┤
│  VK  │ DX12 │ DX11 │   GL   │
│ 后端  │ 后端  │ 后端  │  后端   │
├──────┴──────┴──────┴────────┤
│       Slang 编译器           │
│   (SPIRV/DXIL/DXBC/GLSL)   │
├─────────────────────────────┤
│       GPU 硬件               │
└─────────────────────────────┘
```

## 后端映射表

| RHI 概念 | Vulkan | DX12 | DX11 | OpenGL |
|----------|--------|------|------|--------|
| **实例** | VkInstance | IDXGIFactory6 | IDXGIFactory1 | GL 上下文 (glad) |
| **适配器** | VkPhysicalDevice | IDXGIAdapter4 | IDXGIAdapter1 | GL_RENDERER 字符串 |
| **设备** | VkDevice | ID3D12Device5 | ID3D11Device5 | GL 上下文 |
| **队列** | VkQueue | ID3D12CommandQueue | ID3D11DeviceContext | glFinish (隐式) |
| **命令缓冲区** | VkCommandBuffer | ID3D12GraphicsCommandList | 延迟上下文 | Lambda 列表 |
| **交换链** | VkSwapchainKHR | IDXGISwapChain4 | IDXGISwapChain | SwapBuffers(HDC) |
| **缓冲区** | VkBuffer + VMA | ID3D12Resource | ID3D11Buffer | GLuint |
| **纹理** | VkImage + VMA | ID3D12Resource | ID3D11Texture2D | GLuint |
| **纹理视图** | VkImageView | D3D12 描述符句柄 | ID3D11ShaderResourceView | GLuint (共用) |
| **采样器** | VkSampler | D3D12_SAMPLER_DESC | ID3D11SamplerState | GLuint |
| **描述符集布局** | VkDescriptorSetLayout | 根签名参数 | 绑定槽位元数据 | 绑定信息 |
| **描述符集** | VkDescriptorSet | 描述符堆范围 | 槽位绑定 | GLBindingGroup |
| **管线布局** | VkPipelineLayout | ID3D12RootSignature | 无 | 无 |
| **图形管线** | VkPipeline | ID3D12PipelineState | ID3D11 状态对象组合 | GLuint program + 状态 |
| **管线缓存** | VkPipelineCache | ID3D12PipelineLibrary | 无 | glProgramBinary |
| **着色器模块** | VkShaderModule (SPIRV) | DXIL 字节码 | DXBC 字节码 | GLuint shader (GLSL) |
| **同步** | VkFence + VkSemaphore | ID3D12Fence | ID3D11Query (事件) | GLsync |
| **查询池** | VkQueryPool | ID3D12QueryHeap | ID3D11Query[] | GLuint[] |
| **暂存缓冲区** | VMA 上传分配 | Upload Heap 环形缓冲 | STAGING 缓冲池 | 持久映射缓冲 |
| **屏障** | VkPipelineBarrier | D3D12 ResourceBarrier | 空操作 | glMemoryBarrier |

## 资源状态模型

- **Vulkan/DX12**: 显式状态转换（GPU 强制）
- **DX11/GL**: 隐式状态管理（驱动处理冲突）
- **RHI 规则**: 始终调用 `TransitionImage()` — 现代后端需要，旧后端安全忽略

## 命令录制模型

| 后端 | 模型 | 线程安全 |
|------|------|---------|
| Vulkan | 真正 GPU 命令列表 | 多线程录制，单线程提交 |
| DX12 | 真正 GPU 命令列表 | 多线程录制，单线程提交 |
| DX11 | 立即/延迟上下文 | 单线程 |
| GL | Lambda 捕获 + 延迟回放 | 单线程，Execute() 前后状态隔离 |

## 着色器管线

```
.slang 文件
    │
    ▼ (Slang 编译器)
    ├── Vulkan: SPIR-V 1.5
    ├── DX12:   DXIL (SM 6.6)
    ├── DX11:   DXBC (SM 5.0)
    ├── GL:     GLSL 4.50 (+ Vulkan→GL 修正)
    ├── Metal:  MSL (计划中)
    └── WebGPU: WGSL (计划中)
```

## 后端分级

| 等级 | 后端 | 保证 |
|------|------|------|
| **Tier 1** (完整) | Vulkan, DX12 | 所有 RHI 特性、显式屏障、多队列、管线缓存、异步计算、所有阶段 StorageBuffer 读写 |
| **Tier 2** (兼容) | DX11, OpenGL, GLES | 核心渲染特性、单队列、隐式屏障、部分特性返回 nullptr/空操作 |

### Tier 1 保证
- 真正 GPU 命令列表，多线程录制
- 显式资源状态转换（GPU 强制屏障）
- 多命令队列（图形+计算+传输）
- 管线缓存序列化
- 所有着色器阶段 StorageBuffer 读写
- 完整的描述符集/管线布局绑定校验
- 原生 Timeline Semaphore

### Tier 2 限制
- 命令缓冲区为模拟（DX11: 立即上下文，GL: lambda 回放）
- 屏障为空操作（驱动管理冲突）
- 仅单命令队列
- 无管线缓存序列化
- StorageBuffer 在顶点/片段阶段可能只读（DX11）
- PipelineLayout 返回 nullptr
- Timeline Semaphore 通过事件查询/GL sync 模拟

GL 着色器特殊修正（在 GLShaderModule 中执行）：
- `gl_VertexIndex` → `gl_VertexID`
- `gl_InstanceIndex` → `gl_InstanceID`
