# 着色器指南

## Slang 着色器

Cacao 使用 [Slang](https://shader-slang.org/) 作为着色器语言。编写一个 `.slang` 文件，Cacao 自动编译为各后端格式:

| 后端 | 输出格式 |
|------|----------|
| Vulkan | SPIR-V |
| DX12 | DXIL (SM 6.0) |
| DX11 | DXBC (SM 5.0) |
| OpenGL | GLSL 4.50 |

## 着色器编写

```hlsl
// sprite.slang

struct SpriteInstance {
    float2 position;
    float2 scale;
    float4 uvRect;
    float4 color;
    float rotation;
    float _padding[3];
};

// 使用 Vulkan 风格属性 + DX 风格寄存器
[[vk::binding(0, 0)]]
StructuredBuffer<SpriteInstance> sprites : register(t0);

[[vk::binding(1, 0)]]
Texture2D spriteTexture : register(t1);

[[vk::binding(2, 0)]]
SamplerState spriteSampler : register(s2);

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    // ... 顶点处理 ...
}

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target {
    float4 texColor = spriteTexture.Sample(spriteSampler, input.uv);
    return texColor * input.color;
}
```

## 资源绑定

### StructuredBuffer (存储缓冲区)

```hlsl
StructuredBuffer<MyStruct> data : register(t0);
```

C++ 端:
```cpp
layout.AddBinding(0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex);
descriptorSet->WriteBuffer({0, buffer, 0, sizeof(MyStruct), totalSize, DescriptorType::StorageBuffer});
```

### 纹理 + 采样器

```hlsl
Texture2D myTexture : register(t1);
SamplerState mySampler : register(s2);
```

C++ 端:
```cpp
layout.AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment);
layout.AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment);
descriptorSet->WriteTexture({1, textureView, ResourceState::ShaderRead, DescriptorType::SampledImage});
descriptorSet->WriteSampler({2, sampler});
```

## 着色器编译和缓存

```cpp
auto compiler = instance->CreateShaderCompiler();
compiler->SetCacheDirectory("shader_cache");

ShaderCreateInfo info;
info.SourcePath = "shader.slang";
info.EntryPoint = "mainVS";
info.Stage = ShaderStage::Vertex;
auto shader = compiler->CompileOrLoad(device, info);
```

- 首次运行: Slang 编译着色器并缓存二进制文件
- 后续运行: 从缓存加载 (快速)
- 删除缓存目录强制重新编译
- 每个后端使用独立缓存目录 (`shader_cache_0`, `shader_cache_1` 等)

## 无顶点缓冲区渲染

Cacao 支持仅使用顶点着色器生成几何体:

```hlsl
static const float2 positions[3] = {
    float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.0, 0.5)
};

[shader("vertex")]
float4 mainVS(uint id : SV_VertexID) : SV_Position {
    return float4(positions[id], 0, 1);
}
```

```cpp
cmd->Draw(3, 1, 0, 0);  // 3个顶点，不需要顶点缓冲区
```
