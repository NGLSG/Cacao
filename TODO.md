# Cacao RHI — Feature Roadmap

## Priority 1 — COMPLETED
- [x] **Bindless Descriptors** — BindlessDescriptorPool interface
- [x] **Ray Tracing** — AccelerationStructure + SBT interfaces
- [x] **Mesh Shader** — DispatchMesh command (VK+DX12)
- [x] **DrawIndirectCount** — DrawIndirectCount + DrawIndexedIndirectCount (VK+DX12)

## Priority 2 — COMPLETED
- [x] **Variable Rate Shading (VRS)** — ShadingRateImage interface
- [x] **Sparse Textures** — SparseTexture + TileMapping interface
- [x] **Shader reflection auto layout** — CreateLayoutsFromReflection
- [x] **GPU Profiler** — GPUProfiler with BeginScope/EndScope

## DX12 Optimizations — COMPLETED
- [x] Descriptor heap pooling (65536 CBV/SRV/UAV + 256 RTV + 64 DSV)
- [x] Command allocator pool
- [x] PSO disk cache (ID3D12PipelineLibrary1)
- [x] BindGroup hash caching
- [x] D3D12MA integration (AMD GPUOpen D3D12 Memory Allocator)

## Validation System — COMPLETED
- [x] Pipeline validation (color attachments, MSAA, shaders, layout)
- [x] DescriptorSet StorageBuffer capability check
- [x] Shader-layout mismatch detection
- [x] Debug barrier tracking (GL+DX11: TransitionImage/CopyBufferToImage/BindVertex/BindIndex)
- [x] Buffer usage flag validation

## Infrastructure — COMPLETED
- [x] Auto backend selection
- [x] Capability query system (QueryLimits)
- [x] Backend tier documentation (Tier 1/Tier 2)
- [x] EasyWrapper (CacaoEasy.h)
- [x] Architecture documentation (en+zh)
- [x] DX12 raw benchmark test

## Benchmark Results (RTX 5080, 1M sprites)

| Backend | FPS | Render | Update | Notes |
|---------|-----|--------|--------|-------|
| Vulkan (RHI) | 234 | 0.23ms | 3.4ms | VMA, Fifo present |
| DX12 (RHI) | 210 | 0.18ms | 4.2ms | D3D12MA, SyncInterval=1 |
| DX12 (Raw) | 17 | 0.27ms | 54ms | Single-threaded update |
| OpenGL (RHI) | Pending | | | glad linking issue |

Key findings:
- RHI abstraction overhead: <5% (DX12 raw render 0.27ms vs RHI 0.18ms — RHI is *faster* due to optimizations)
- VK vs DX12 FPS gap: ~15% — driven by CPU-side driver scheduling, not RHI
- DX12 GPU render is 22% faster than VK (0.18 vs 0.23ms)
- Update bottleneck is CPU computation, not GPU API

## Future (Planned)
- [ ] Metal Backend (macOS/iOS)
- [ ] WebGPU Backend (Dawn/wgpu)
- [ ] Render Graph
- [ ] Unit test suite
- [ ] CI/CD pipeline
- [ ] GPU Compute sprite update (move Update to Compute Shader)
