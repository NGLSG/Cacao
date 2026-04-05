# 后端特性说明

## Vulkan

- 功能最完整的后端，支持验证层
- SPIR-V 着色器
- 显式同步 (fence + semaphore)
- 多队列支持 (图形/计算/传输)
- VMA 内存分配器

## DirectX 12

- 仅 Windows，需要 Windows 10+
- DXIL 着色器 (SM 6.6)，支持光线追踪和 Mesh Shader
- 根签名从 DescriptorSetLayout 自动生成
- 每帧独立命令分配器，避免同步冲突
- 队列缓存，避免交换链/队列不匹配
- `StorageBuffer` 映射为 SRV (非 UAV)，兼容 StructuredBuffer

## DirectX 11

- 仅 Windows，比 DX12 简单
- DXBC 着色器 (SM 5.0)
- 立即上下文模型（命令立即执行）
- FLIP_DISCARD 交换链 — 只能通过 GetBuffer(0) 获取后缓冲
- StorageBuffer 使用影子结构化缓冲区创建 SRV
- 缓冲数据通过 CPU staging + UpdateSubresource 同步

**限制:**
- 顶点/像素着色器阶段无法使用 StorageBuffer 的 UAV
- 单命令队列（无异步计算）
- PipelineLayout 返回 nullptr（DX11 不需要）

## OpenGL 4.5

- 跨平台（Windows WGL, Linux GLX/EGL）
- GLSL 4.50 着色器，Slang 输出后自动修正:
  - `gl_VertexIndex` → `gl_VertexID`
  - `gl_InstanceIndex` → `gl_InstanceID`
  - Vulkan 分离纹理/采样器 → GL 合并 sampler2D
- 延迟命令录制（存储为 lambda，预分配容量减少堆分配，Submit 时执行）
- GL 状态隔离：Execute() 前后保存/恢复 VAO、FBO、Program
- 无显式渲染目标时使用默认帧缓冲 (FBO 0)
- 缓冲数据通过 CPU staging + glBufferSubData 同步
- glad 加载器加载 GL 函数

**限制:**
- 无显式屏障（驱动管理资源冲突）
- 无管线缓存序列化
- 纹理上传使用 glTexSubImage2D（CPU 指针方式）

## OpenGL ES 3.2

- 与 GL 后端共用代码 (`#ifdef CACAO_GLES`)
- 需要 EGL 上下文（替代 WGL）
- 使用 `<GLES3/gl32.h>` 替代 glad
- 设计用于 Android/iOS
- Windows 上需要 ANGLE

## Metal (规划中)

- macOS/iOS
- MSL 着色器
- 显式资源管理，类似 DX12

## Metal

- macOS 10.15+ / iOS 13+
- MSL 着色器（通过 Slang 编译）
- 自动资源追踪，无需手动 barrier
- Apple Silicon 光线追踪（macOS 11+）
- `BackendType::Metal`

## WebGPU

- 跨平台（Windows/macOS/Linux/Web）
- WGSL 着色器（通过 Slang 编译）
- 自动资源状态管理，barrier 为空操作
- 不支持光线追踪和 push constants
- 桌面端通过 Dawn 或 wgpu-native 实现
- `BackendType::WebGPU`
