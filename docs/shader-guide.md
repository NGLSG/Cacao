# Cacao Shader Guide

Cacao uses **Slang** as its shader language, providing a single source that compiles to SPIR-V (Vulkan), DXIL (DX12), DXBC (DX11), and GLSL (OpenGL) automatically.

## Slang Basics

Slang is a shader language with HLSL-compatible syntax and additional features like generics, interfaces, and modules. Cacao's `ShaderCompiler` handles the compilation pipeline.

### Shader File Structure

A typical `.slang` shader file:

```hlsl
// vertex & fragment in one file: my_shader.slang

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 worldNormal : NORMAL;
};

// Resources
cbuffer SceneData : register(b0, space0)
{
    float4x4 viewProjection;
    float4x4 model;
    float3 lightDir;
};

Texture2D albedoTexture : register(t1, space0);
SamplerState linearSampler : register(s2, space0);

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    output.position = mul(viewProjection, worldPos);
    output.texcoord = input.texcoord;
    output.worldNormal = mul((float3x3)model, input.normal);
    return output;
}

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target
{
    float3 albedo = albedoTexture.Sample(linearSampler, input.texcoord).rgb;
    float ndotl = max(dot(normalize(input.worldNormal), lightDir), 0.0);
    return float4(albedo * ndotl, 1.0);
}
```

## Compiling Shaders

### Using ShaderCompiler

```cpp
// Create compiler from instance
auto compiler = instance->CreateShaderCompiler();
compiler->SetCacheDirectory("shader_cache");

// Compile vertex shader
ShaderCreateInfo vsInfo;
vsInfo.SourcePath = "my_shader.slang";
vsInfo.EntryPoint = "mainVS";
vsInfo.Stage = ShaderStage::Vertex;
auto vertexShader = compiler->CompileOrLoad(device, vsInfo);

// Compile fragment shader
ShaderCreateInfo fsInfo;
fsInfo.SourcePath = "my_shader.slang";
fsInfo.EntryPoint = "mainPS";
fsInfo.Stage = ShaderStage::Fragment;
auto fragmentShader = compiler->CompileOrLoad(device, fsInfo);
```

### ShaderCreateInfo

```cpp
struct ShaderCreateInfo {
    std::string SourcePath;          // Path to .slang file
    std::string EntryPoint = "main"; // Entry point function name
    ShaderStage Stage;               // Vertex, Fragment, Compute, etc.
    ShaderDefines Defines;           // Preprocessor defines (map<string, string>)
    std::string Profile;             // Target profile override
};
```

### Shader Stages

```cpp
enum class ShaderStage : uint32_t {
    Vertex, Fragment, Compute,
    Geometry, TessellationControl, TessellationEvaluation,
    RayGen, RayAnyHit, RayClosestHit, RayMiss, RayIntersection, Callable,
    Mesh, Task
};
```

### Preprocessor Defines

```cpp
ShaderCreateInfo info;
info.SourcePath = "my_shader.slang";
info.EntryPoint = "mainVS";
info.Stage = ShaderStage::Vertex;
info.Defines = {
    {"USE_NORMAL_MAP", "1"},
    {"MAX_LIGHTS", "16"}
};
auto shader = compiler->CompileOrLoad(device, info);
```

## Shader Caching

`CompileOrLoad` automatically caches compiled shader bytecode to disk. On subsequent runs, the cached binary is loaded directly if the source file hasn't changed.

```cpp
compiler->SetCacheDirectory("shader_cache_vulkan");

// First call: compiles and caches
auto shader = compiler->CompileOrLoad(device, info);

// Subsequent calls: loads from cache
auto shader2 = compiler->CompileOrLoad(device, info);

// Clean stale entries
compiler->PruneCache();
```

The cache key includes: source path, entry point, stage, defines, and backend type.

## Resource Binding

### Uniform Buffers (Constant Buffers)

```hlsl
// Slang
cbuffer PerFrame : register(b0, space0)
{
    float4x4 viewProjection;
    float time;
};
```

```cpp
// C++ - matches binding 0, set 0
layout.AddBinding(0, DescriptorType::UniformBuffer, 1, ShaderStage::Vertex);
```

### Structured Buffers (SSBO)

```hlsl
// Slang - read-only
StructuredBuffer<SpriteInstance> sprites : register(t0, space0);

// Slang - read-write
RWStructuredBuffer<Particle> particles : register(u0, space0);
```

```cpp
// C++ - StructuredBuffer maps to StorageBuffer
layout.AddBinding(0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex);
```

### Textures

```hlsl
// Separate texture and sampler (recommended)
Texture2D albedoTex : register(t1, space0);
SamplerState linearSampler : register(s2, space0);

// Usage in shader
float4 color = albedoTex.Sample(linearSampler, uv);
```

```cpp
// C++ - separate bindings
layout.AddBinding(1, DescriptorType::SampledImage, 1, ShaderStage::Fragment);
layout.AddBinding(2, DescriptorType::Sampler, 1, ShaderStage::Fragment);
```

### Storage Images

```hlsl
RWTexture2D<float4> outputImage : register(u0, space0);

[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 tid : SV_DispatchThreadID)
{
    outputImage[tid.xy] = float4(tid.xy / 512.0, 0.0, 1.0);
}
```

```cpp
layout.AddBinding(0, DescriptorType::StorageImage, 1, ShaderStage::Compute);
```

### Register Convention

Slang uses HLSL-style register syntax that maps to Cacao's descriptor types:

| Register | Cacao DescriptorType | Usage |
|---|---|---|
| `b#` | `UniformBuffer` | Constant buffers |
| `t#` | `SampledImage` or `StorageBuffer` (read-only) | Textures, StructuredBuffer |
| `u#` | `StorageImage` or `StorageBuffer` (read-write) | UAVs, RWStructuredBuffer |
| `s#` | `Sampler` | Sampler states |

The `space#` maps to the descriptor set index.

## Compute Shaders

```hlsl
// sprite_update.slang

struct SpriteInstance
{
    float2 position;
    float2 scale;
    float4 uvRect;
    float4 color;
    float rotation;
    float3 _padding;
};

RWStructuredBuffer<SpriteInstance> sprites : register(u0, space0);

cbuffer UpdateParams : register(b1, space0)
{
    float deltaTime;
    float time;
    uint spriteCount;
};

[shader("compute")]
[numthreads(256, 1, 1)]
void mainCS(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= spriteCount) return;

    SpriteInstance s = sprites[tid.x];
    s.position.x += sin(time + tid.x * 0.01) * deltaTime * 0.1;
    s.position.y += cos(time + tid.x * 0.02) * deltaTime * 0.1;
    s.rotation += deltaTime;
    sprites[tid.x] = s;
}
```

```cpp
// C++ dispatch
ShaderCreateInfo csInfo;
csInfo.Stage = ShaderStage::Compute;
csInfo.SourcePath = "sprite_update.slang";
csInfo.EntryPoint = "mainCS";
auto computeShader = compiler->CompileOrLoad(device, csInfo);

auto computePipeline = device->CreateComputePipeline(
    ComputePipelineBuilder()
        .SetShader(computeShader)
        .SetLayout(computeLayout)
        .Build());

cmd->BindComputePipeline(computePipeline);
cmd->BindComputeDescriptorSets(computePipeline, 0, {descSet});
cmd->Dispatch((spriteCount + 255) / 256, 1, 1);
```

## Vertex-less Rendering (Programmable Pulling)

Cacao supports vertex-less rendering where vertex data is pulled from storage buffers:

```hlsl
StructuredBuffer<SpriteInstance> sprites : register(t0, space0);

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    // 6 vertices per sprite quad (2 triangles)
    uint quadVertex = vertexID % 6;
    SpriteInstance sprite = sprites[instanceID];

    float2 corners[6] = {
        float2(-0.5, -0.5), float2( 0.5, -0.5), float2( 0.5,  0.5),
        float2(-0.5, -0.5), float2( 0.5,  0.5), float2(-0.5,  0.5)
    };
    // ... transform using sprite data
}
```

```cpp
// No vertex buffer binding needed!
cmd->BindGraphicsPipeline(pipeline);
cmd->BindDescriptorSets(pipeline, 0, {descriptorSet});
cmd->Draw(6, spriteCount, 0, 0);  // 6 verts per quad, N instances
```

## Shader Reflection

The compiler provides automatic reflection data for validating shader-layout compatibility:

```cpp
auto shader = compiler->CompileOrLoad(device, info);
const auto& reflection = shader->GetReflection();

// Inspect bindings
for (const auto& binding : reflection.ResourceBindings) {
    std::cout << "Set " << binding.Set
              << " Binding " << binding.Binding
              << " Name: " << binding.Name << std::endl;
}

// Auto-create descriptor layouts from reflection
auto layouts = ShaderCompiler::CreateLayoutsFromReflection(reflection);

// Validate layout compatibility
std::string error;
if (!ShaderCompiler::ValidateShaderAgainstLayout(reflection, layoutInfo, error)) {
    std::cerr << "Layout mismatch: " << error << std::endl;
}
```

## Ray Tracing Shaders

Ray tracing shaders are compiled as a library with multiple entry points:

```hlsl
// cornell_box.slang

struct RayPayload { float3 color; float distance; };

[shader("raygeneration")]
void RayGen()
{
    // Generate primary rays
    RayDesc ray;
    ray.Origin = cameraPos;
    ray.Direction = computeRayDir(DispatchRaysIndex().xy, DispatchRaysDimensions().xy);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    RayPayload payload;
    TraceRay(accelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    outputImage[DispatchRaysIndex().xy] = float4(payload.color, 1.0);
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.color = float3(attr.barycentrics, 1.0 - attr.barycentrics.x - attr.barycentrics.y);
    payload.distance = RayTCurrent();
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
    payload.distance = -1.0;
}
```

```cpp
auto rtShader = compiler->CompileLibrary(
    device,
    "cornell_box.slang",
    {"RayGen", "ClosestHit", "Miss"});
```

## Cross-Backend Tips

1. **Use `space` for descriptor sets**: `register(t0, space0)` maps to set 0, binding 0.
2. **Avoid GLSL-specific features**: Stick to HLSL/Slang syntax for portability.
3. **Use `[numthreads]`**: Always specify work group size in compute shaders.
4. **Push constants**: Use Slang's `[[vk::push_constant]]` or keep small constants in a UBO for DX compatibility.
5. **Samplers and textures**: Use separate `Texture2D` + `SamplerState` instead of combined image samplers for best cross-backend compatibility.
6. **Row-major matrices**: Be explicit about matrix layout with `row_major` or `column_major` if needed.
