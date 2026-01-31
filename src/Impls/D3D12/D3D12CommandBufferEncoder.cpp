#ifdef WIN32
#include "Impls/D3D12/D3D12CommandBufferEncoder.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Pipeline.h"
#include "Impls/D3D12/D3D12PipelineLayout.h"
#include "Impls/D3D12/D3D12Buffer.h"
#include "Impls/D3D12/D3D12Texture.h"
#include "Impls/D3D12/D3D12DescriptorSet.h"
#include "Impls/D3D12/D3D12DescriptorPool.h"
#include <stdexcept>

namespace Cacao
{
    // Helper function to get bytes per pixel for a format
    static uint32_t GetFormatBytesPerPixel(Format format)
    {
        switch (format)
        {
            // 1 byte per pixel
            case Format::R8_UNORM:
            case Format::R8_SNORM:
            case Format::R8_UINT:
            case Format::R8_SINT:
                return 1;
            
            // 2 bytes per pixel
            case Format::RG8_UNORM:
            case Format::RG8_SNORM:
            case Format::RG8_UINT:
            case Format::RG8_SINT:
            case Format::R16_UNORM:
            case Format::R16_SNORM:
            case Format::R16_UINT:
            case Format::R16_SINT:
            case Format::R16_FLOAT:
            case Format::D16_UNORM:
                return 2;
            
            // 4 bytes per pixel
            case Format::RGBA8_UNORM:
            case Format::RGBA8_SNORM:
            case Format::RGBA8_UINT:
            case Format::RGBA8_SINT:
            case Format::RGBA8_SRGB:
            case Format::BGRA8_UNORM:
            case Format::BGRA8_SRGB:
            case Format::RG16_UNORM:
            case Format::RG16_SNORM:
            case Format::RG16_UINT:
            case Format::RG16_SINT:
            case Format::RG16_FLOAT:
            case Format::R32_UINT:
            case Format::R32_SINT:
            case Format::R32_FLOAT:
            case Format::RGB10A2_UNORM:
            case Format::RGB10A2_UINT:
            case Format::RG11B10_FLOAT:
            case Format::RGB9E5_FLOAT:
            case Format::D32_FLOAT:
            case Format::D24_UNORM_S8_UINT:
                return 4;
            
            // 8 bytes per pixel
            case Format::RGBA16_UNORM:
            case Format::RGBA16_SNORM:
            case Format::RGBA16_UINT:
            case Format::RGBA16_SINT:
            case Format::RGBA16_FLOAT:
            case Format::RG32_UINT:
            case Format::RG32_SINT:
            case Format::RG32_FLOAT:
            case Format::D32_FLOAT_S8_UINT:
                return 8;
            
            // 12 bytes per pixel
            case Format::RGB32_UINT:
            case Format::RGB32_SINT:
            case Format::RGB32_FLOAT:
                return 12;
            
            // 16 bytes per pixel
            case Format::RGBA32_UINT:
            case Format::RGBA32_SINT:
            case Format::RGBA32_FLOAT:
                return 16;
            
            // BC compressed formats (block size)
            case Format::BC1_RGB_UNORM:
            case Format::BC1_RGB_SRGB:
            case Format::BC1_RGBA_UNORM:
            case Format::BC1_RGBA_SRGB:
            case Format::BC4_UNORM:
            case Format::BC4_SNORM:
                return 8; // 8 bytes per 4x4 block
            
            case Format::BC2_UNORM:
            case Format::BC2_SRGB:
            case Format::BC3_UNORM:
            case Format::BC3_SRGB:
            case Format::BC5_UNORM:
            case Format::BC5_SNORM:
            case Format::BC6H_UFLOAT:
            case Format::BC6H_SFLOAT:
            case Format::BC7_UNORM:
            case Format::BC7_SRGB:
                return 16; // 16 bytes per 4x4 block
            
            default:
                return 4; // Default to 4 bytes
        }
    }
    D3D12CommandBufferEncoder::D3D12CommandBufferEncoder(const Ref<Device>& device,
                                                         ComPtr<ID3D12GraphicsCommandList> commandList,
                                                         ID3D12CommandAllocator* allocator,
                                                         CommandBufferType type)
        : m_allocator(allocator), m_type(type)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12CommandBufferEncoder created with null device");
        }
        m_device = std::dynamic_pointer_cast<D3D12Device>(device);

        // 获取 ID3D12GraphicsCommandList7 接口
        HRESULT hr = commandList->QueryInterface(IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get ID3D12GraphicsCommandList7");
        }
    }

    Ref<D3D12CommandBufferEncoder> D3D12CommandBufferEncoder::Create(const Ref<Device>& device,
                                                                     ComPtr<ID3D12GraphicsCommandList> commandList,
                                                                     ID3D12CommandAllocator* allocator,
                                                                     CommandBufferType type)
    {
        return CreateRef<D3D12CommandBufferEncoder>(device, commandList, allocator, type);
    }

    void D3D12CommandBufferEncoder::Free()
    {
        m_device->FreeCommandBuffer(shared_from_this());
    }

    void D3D12CommandBufferEncoder::Reset()
    {
        m_device->ResetCommandBuffer(shared_from_this());
    }

    void D3D12CommandBufferEncoder::ReturnToPool()
    {
        m_device->ReturnCommandBuffer(shared_from_this());
    }

    void D3D12CommandBufferEncoder::Begin(const CommandBufferBeginInfo& info)
    {
        // D3D12 命令列表在创建时已经处于录制状态
        // 如果需要重用，需要先 Reset
        m_commandList->Reset(m_allocator, nullptr);
    }

    void D3D12CommandBufferEncoder::End()
    {
        m_commandList->Close();
    }

    void D3D12CommandBufferEncoder::BeginRendering(const RenderingInfo& info)
    {
        m_isRendering = true;
        m_currentRTVs.clear();
        m_hasDSV = false;

        // 收集渲染目标
        for (const auto& colorAttachment : info.ColorAttachments)
        {
            if (!colorAttachment.Texture) continue;

            auto* d3d12Texture = static_cast<D3D12Texture*>(colorAttachment.Texture.get());
            auto* view = static_cast<D3D12TextureView*>(d3d12Texture->GetDefaultView().get());

            if (view && view->HasRTV())
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtv = view->GetRTVHandle();
                m_currentRTVs.push_back(rtv);

                // 清除渲染目标
                if (colorAttachment.LoadOp == AttachmentLoadOp::Clear)
                {
                    m_commandList->ClearRenderTargetView(rtv, colorAttachment.ClearValue.Color, 0, nullptr);
                }
            }
        }

        // 处理深度附件
        if (info.DepthAttachment && info.DepthAttachment->Texture)
        {
            auto* d3d12Texture = static_cast<D3D12Texture*>(info.DepthAttachment->Texture.get());
            auto* view = static_cast<D3D12TextureView*>(d3d12Texture->GetDefaultView().get());

            if (view && view->HasDSV())
            {
                m_currentDSV = view->GetDSVHandle();
                m_hasDSV = true;

                if (info.DepthAttachment->LoadOp == AttachmentLoadOp::Clear)
                {
                    D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
                    if (info.StencilAttachment) flags |= D3D12_CLEAR_FLAG_STENCIL;

                    m_commandList->ClearDepthStencilView(
                        m_currentDSV, flags,
                        info.DepthAttachment->ClearDepthStencil.Depth,
                        static_cast<UINT8>(info.DepthAttachment->ClearDepthStencil.Stencil),
                        0, nullptr);
                }
            }
        }

        // 设置渲染目标
        m_commandList->OMSetRenderTargets(
            static_cast<UINT>(m_currentRTVs.size()),
            m_currentRTVs.empty() ? nullptr : m_currentRTVs.data(),
            FALSE,
            m_hasDSV ? &m_currentDSV : nullptr);
    }

    void D3D12CommandBufferEncoder::EndRendering()
    {
        m_isRendering = false;
        m_currentRTVs.clear();
        m_hasDSV = false;
    }

    void D3D12CommandBufferEncoder::BindGraphicsPipeline(const Ref<GraphicsPipeline>& pipeline)
    {
        auto* d3d12Pipeline = static_cast<D3D12GraphicsPipeline*>(pipeline.get());
        m_boundGraphicsPipeline = d3d12Pipeline;

        m_commandList->SetPipelineState(d3d12Pipeline->GetPipelineState());
        m_commandList->IASetPrimitiveTopology(d3d12Pipeline->GetPrimitiveTopology());

        auto* layout = static_cast<D3D12PipelineLayout*>(d3d12Pipeline->GetLayout().get());
        m_commandList->SetGraphicsRootSignature(layout->GetRootSignature().Get());
    }

    void D3D12CommandBufferEncoder::BindComputePipeline(const Ref<ComputePipeline>& pipeline)
    {
        auto* d3d12Pipeline = static_cast<D3D12ComputePipeline*>(pipeline.get());
        m_boundComputePipeline = d3d12Pipeline;

        m_commandList->SetPipelineState(d3d12Pipeline->GetPipelineState());

        auto* layout = static_cast<D3D12PipelineLayout*>(d3d12Pipeline->GetLayout().get());
        m_commandList->SetComputeRootSignature(layout->GetRootSignature().Get());
    }

    void D3D12CommandBufferEncoder::SetViewport(const Viewport& viewport)
    {
        D3D12_VIEWPORT d3d12Viewport = {};
        d3d12Viewport.TopLeftX = viewport.X;
        d3d12Viewport.TopLeftY = viewport.Y;
        d3d12Viewport.Width = viewport.Width;
        d3d12Viewport.Height = viewport.Height;
        d3d12Viewport.MinDepth = viewport.MinDepth;
        d3d12Viewport.MaxDepth = viewport.MaxDepth;

        m_commandList->RSSetViewports(1, &d3d12Viewport);
    }

    void D3D12CommandBufferEncoder::SetScissor(const Rect2D& scissor)
    {
        D3D12_RECT d3d12Rect = {};
        d3d12Rect.left = scissor.OffsetX;
        d3d12Rect.top = scissor.OffsetY;
        d3d12Rect.right = scissor.OffsetX + static_cast<LONG>(scissor.Width);
        d3d12Rect.bottom = scissor.OffsetY + static_cast<LONG>(scissor.Height);

        m_commandList->RSSetScissorRects(1, &d3d12Rect);
    }

    void D3D12CommandBufferEncoder::BindVertexBuffer(uint32_t binding, const Ref<Buffer>& buffer, uint64_t offset)
    {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(buffer.get());

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = d3d12Buffer->GetGPUVirtualAddress() + offset;
        vbv.SizeInBytes = static_cast<UINT>(d3d12Buffer->GetSize() - offset);
        vbv.StrideInBytes = 0; // 注意：如果使用 Vertex Buffer，这里必须设置正确的 Stride

        m_commandList->IASetVertexBuffers(binding, 1, &vbv);
    }

    void D3D12CommandBufferEncoder::BindIndexBuffer(const Ref<Buffer>& buffer, uint64_t offset, IndexType indexType)
    {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(buffer.get());

        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = d3d12Buffer->GetGPUVirtualAddress() + offset;
        ibv.SizeInBytes = static_cast<UINT>(d3d12Buffer->GetSize() - offset);
        ibv.Format = D3D12Converter::Convert(indexType);

        m_commandList->IASetIndexBuffer(&ibv);
    }

    void D3D12CommandBufferEncoder::BindDescriptorSets(const Ref<GraphicsPipeline>& pipeline, uint32_t firstSet,
                                                       std::span<const Ref<DescriptorSet>> descriptorSets)
    {
        auto* d3d12Pipeline = static_cast<D3D12GraphicsPipeline*>(pipeline.get());
        auto* layout = static_cast<D3D12PipelineLayout*>(d3d12Pipeline->GetLayout().get());

        // =========================================================================================
        // [修复核心]：必须先设置 Descriptor Heaps，否则 SetGraphicsRootDescriptorTable 会导致 GPU 崩溃
        // =========================================================================================

        // D3D12 限制：每次 SetDescriptorHeaps 只能绑定一个 CBV/SRV/UAV 堆和一个 Sampler 堆
        // 我们假设这一批 Sets 都来自同一个 Pool (通常架构如此设计)
        if (!descriptorSets.empty())
        {
            auto* firstD3D12Set = static_cast<D3D12DescriptorSet*>(descriptorSets[0].get());

            ID3D12DescriptorHeap* heaps[2] = {};
            UINT heapCount = 0;

            ID3D12DescriptorHeap* cbvSrvHeap = firstD3D12Set->GetCbvSrvUavHeap();
            ID3D12DescriptorHeap* samplerHeap = firstD3D12Set->GetSamplerHeap();

            if (cbvSrvHeap) heaps[heapCount++] = cbvSrvHeap;
            if (samplerHeap) heaps[heapCount++] = samplerHeap;

            if (heapCount > 0)
            {
                m_commandList->SetDescriptorHeaps(heapCount, heaps);
            }
        }

        // =========================================================================================

        for (uint32_t i = 0; i < descriptorSets.size(); ++i)
        {
            auto* d3d12Set = static_cast<D3D12DescriptorSet*>(descriptorSets[i].get());

            // 获取该 set 对应的 root parameter 起始索引
            uint32_t rootParamIndex = layout->GetSetRootParamStartIndex(firstSet + i);

            // 绑定 CBV/SRV/UAV 描述符表
            D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavHandle = d3d12Set->GetCbvSrvUavGpuHandle();
            if (cbvSrvUavHandle.ptr != 0)
            {
                // 这里就是之前崩溃的地方，现在因为 SetDescriptorHeaps 已调用，应该安全了
                m_commandList->SetGraphicsRootDescriptorTable(rootParamIndex, cbvSrvUavHandle);
                rootParamIndex++;
            }

            // 绑定 Sampler 描述符表
            D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = d3d12Set->GetSamplerGpuHandle();
            if (samplerHandle.ptr != 0)
            {
                m_commandList->SetGraphicsRootDescriptorTable(rootParamIndex, samplerHandle);
            }
        }
    }

    void D3D12CommandBufferEncoder::PushConstants(const Ref<GraphicsPipeline>& pipeline, ShaderStage stageFlags,
                                                  uint32_t offset, uint32_t size, const void* data)
    {
        auto* layout = static_cast<D3D12PipelineLayout*>(pipeline->GetLayout().get());
        uint32_t rootIndex = layout->GetPushConstantsRootIndex();

        if (rootIndex != UINT32_MAX)
        {
            m_commandList->SetGraphicsRoot32BitConstants(
                rootIndex, size / 4, data, offset / 4);
        }
    }

    void D3D12CommandBufferEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount,
                                         uint32_t firstVertex, uint32_t firstInstance)
    {
        m_commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void D3D12CommandBufferEncoder::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                                uint32_t firstIndex, int32_t vertexOffset,
                                                uint32_t firstInstance)
    {
        m_commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void D3D12CommandBufferEncoder::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void D3D12CommandBufferEncoder::BindComputeDescriptorSets(const Ref<ComputePipeline>& pipeline, uint32_t firstSet,
                                                              std::span<const Ref<DescriptorSet>> descriptorSets)
    {
        auto* d3d12Pipeline = static_cast<D3D12ComputePipeline*>(pipeline.get());
        auto* layout = static_cast<D3D12PipelineLayout*>(d3d12Pipeline->GetLayout().get());

        // =========================================================================================
        // [修复] 必须先设置 Descriptor Heaps，否则 SetComputeRootDescriptorTable 会导致 GPU 崩溃
        // =========================================================================================
        if (!descriptorSets.empty())
        {
            auto* firstD3D12Set = static_cast<D3D12DescriptorSet*>(descriptorSets[0].get());

            ID3D12DescriptorHeap* heaps[2] = {};
            UINT heapCount = 0;

            ID3D12DescriptorHeap* cbvSrvHeap = firstD3D12Set->GetCbvSrvUavHeap();
            ID3D12DescriptorHeap* samplerHeap = firstD3D12Set->GetSamplerHeap();

            if (cbvSrvHeap) heaps[heapCount++] = cbvSrvHeap;
            if (samplerHeap) heaps[heapCount++] = samplerHeap;

            if (heapCount > 0)
            {
                m_commandList->SetDescriptorHeaps(heapCount, heaps);
            }
        }
        // =========================================================================================

        for (uint32_t i = 0; i < descriptorSets.size(); ++i)
        {
            auto* d3d12Set = static_cast<D3D12DescriptorSet*>(descriptorSets[i].get());

            // 获取该 set 对应的 root parameter 起始索引
            uint32_t rootParamIndex = layout->GetSetRootParamStartIndex(firstSet + i);

            // 绑定 CBV/SRV/UAV 描述符表
            D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavHandle = d3d12Set->GetCbvSrvUavGpuHandle();
            if (cbvSrvUavHandle.ptr != 0)
            {
                m_commandList->SetComputeRootDescriptorTable(rootParamIndex, cbvSrvUavHandle);
                rootParamIndex++;
            }

            // 绑定 Sampler 描述符表
            D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = d3d12Set->GetSamplerGpuHandle();
            if (samplerHandle.ptr != 0)
            {
                m_commandList->SetComputeRootDescriptorTable(rootParamIndex, samplerHandle);
            }
        }
    }

    void D3D12CommandBufferEncoder::ComputePushConstants(const Ref<ComputePipeline>& pipeline, ShaderStage stageFlags,
                                                         uint32_t offset, uint32_t size, const void* data)
    {
        auto* layout = static_cast<D3D12PipelineLayout*>(pipeline->GetLayout().get());
        uint32_t rootIndex = layout->GetPushConstantsRootIndex();

        if (rootIndex != UINT32_MAX)
        {
            m_commandList->SetComputeRoot32BitConstants(rootIndex, size / 4, data, offset / 4);
        }
    }

    void D3D12CommandBufferEncoder::PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                                                    std::span<const CMemoryBarrier> globalBarriers,
                                                    std::span<const BufferBarrier> bufferBarriers,
                                                    std::span<const TextureBarrier> textureBarriers)
    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;

        for (const auto& texBarrier : textureBarriers)
        {
            auto* d3d12Texture = static_cast<D3D12Texture*>(texBarrier.Texture.get());

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = d3d12Texture->GetResource();
            barrier.Transition.StateBefore = d3d12Texture->GetCurrentState();
            barrier.Transition.StateAfter = D3D12Converter::Convert(texBarrier.NewLayout);
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            if (barrier.Transition.StateBefore != barrier.Transition.StateAfter)
            {
                barriers.push_back(barrier);
                d3d12Texture->SetCurrentState(barrier.Transition.StateAfter);
            }
        }

        if (!barriers.empty())
        {
            m_commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }
    }

    void D3D12CommandBufferEncoder::TransitionImage(const Ref<Texture>& texture, ImageTransition transition,
                                                    const ImageSubresourceRange& range)
    {
        auto* d3d12Texture = static_cast<D3D12Texture*>(texture.get());
        D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COMMON;

        switch (transition)
        {
        case ImageTransition::UndefinedToColorAttachment:
        case ImageTransition::PresentToColorAttachment:
            newState = D3D12_RESOURCE_STATE_RENDER_TARGET;
            break;
        case ImageTransition::ColorAttachmentToPresent:
            newState = D3D12_RESOURCE_STATE_PRESENT;
            break;
        case ImageTransition::UndefinedToShaderRead:
        case ImageTransition::ColorAttachmentToShaderRead:
        case ImageTransition::TransferDstToShaderRead:
            newState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            break;
        case ImageTransition::UndefinedToTransferDst:
        case ImageTransition::ShaderReadToTransferDst:
            newState = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        case ImageTransition::ColorAttachmentToTransferSrc:
        case ImageTransition::ShaderReadToTransferSrc:
            newState = D3D12_RESOURCE_STATE_COPY_SOURCE;
            break;
        case ImageTransition::UndefinedToDepthAttachment:
            newState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            break;
        default:
            newState = D3D12_RESOURCE_STATE_COMMON;
            break;
        }

        D3D12_RESOURCE_STATES currentState = d3d12Texture->GetCurrentState();
        if (currentState != newState)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = d3d12Texture->GetResource();
            barrier.Transition.StateBefore = currentState;
            barrier.Transition.StateAfter = newState;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            m_commandList->ResourceBarrier(1, &barrier);
            d3d12Texture->SetCurrentState(newState);
        }
    }

    void D3D12CommandBufferEncoder::TransitionBuffer(const Ref<Buffer>& buffer, BufferTransition transition,
                                                     uint64_t offset, uint64_t size)
    {
        // D3D12 缓冲区通常不需要显式转换
    }

    void D3D12CommandBufferEncoder::MemoryBarrierFast(MemoryTransition transition)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = nullptr;
        m_commandList->ResourceBarrier(1, &barrier);
    }

    void D3D12CommandBufferEncoder::ExecuteNative(const std::function<void(void*)>& func)
    {
        func(m_commandList.Get());
    }

    void* D3D12CommandBufferEncoder::GetNativeHandle()
    {
        return m_commandList.Get();
    }

    void D3D12CommandBufferEncoder::CopyBufferToImage(const Ref<Buffer>& srcBuffer, const Ref<Texture>& dstImage,
                                                      ImageLayout dstImageLayout,
                                                      std::span<const BufferImageCopy> regions)
    {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(srcBuffer.get());
        auto* d3d12Texture = static_cast<D3D12Texture*>(dstImage.get());

        for (const auto& region : regions)
        {
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = d3d12Texture->GetResource();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = region.ImageSubresource.MipLevel +
                region.ImageSubresource.BaseArrayLayer * dstImage->GetMipLevels();

            // Calculate bytes per pixel based on format
            uint32_t bytesPerPixel = GetFormatBytesPerPixel(dstImage->GetFormat());
            
            // Calculate row pitch based on actual buffer data layout
            // NOTE: D3D12 requires RowPitch to be aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256 bytes).
            // If your staging buffer has tightly-packed pixel data and the width doesn't naturally align,
            // you MUST pad your staging buffer data row-by-row to match the aligned pitch.
            // For example: a 100-wide RGBA8 texture has natural pitch of 400 bytes, but D3D12 requires 512.
            // In this case, you must allocate 512 * height bytes and copy each row with 512-byte stride.
            uint32_t rowPitch;
            if (region.BufferRowLength > 0)
            {
                // User provided explicit row length - use it (assumes they've aligned it properly)
                rowPitch = region.BufferRowLength;
            }
            else
            {
                // Tightly packed data - use natural pitch
                rowPitch = region.ImageExtentWidth * bytesPerPixel;
            }
            
            // D3D12 requires 256-byte aligned RowPitch - align it
            // WARNING: This assumes the staging buffer data is ALSO laid out with this alignment!
            rowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);

            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = d3d12Buffer->GetHandle();
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint.Offset = region.BufferOffset;
            src.PlacedFootprint.Footprint.Format = D3D12Converter::Convert(dstImage->GetFormat());
            src.PlacedFootprint.Footprint.Width = region.ImageExtentWidth;
            src.PlacedFootprint.Footprint.Height = region.ImageExtentHeight;
            src.PlacedFootprint.Footprint.Depth = region.ImageExtentDepth;
            src.PlacedFootprint.Footprint.RowPitch = rowPitch;

            D3D12_BOX srcBox = {};
            srcBox.left = 0;
            srcBox.top = 0;
            srcBox.front = 0;
            srcBox.right = region.ImageExtentWidth;
            srcBox.bottom = region.ImageExtentHeight;
            srcBox.back = region.ImageExtentDepth;

            m_commandList->CopyTextureRegion(&dst,
                                             region.ImageOffsetX, region.ImageOffsetY, region.ImageOffsetZ,
                                             &src, &srcBox);
        }
    }

    CommandBufferType D3D12CommandBufferEncoder::GetCommandBufferType() const
    {
        return m_type;
    }

    void D3D12CommandBufferEncoder::CopyBuffer(const Ref<Buffer>& srcBuffer, const Ref<Buffer>& dstBuffer,
                                               uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        auto* d3d12SrcBuffer = static_cast<D3D12Buffer*>(srcBuffer.get());
        auto* d3d12DstBuffer = static_cast<D3D12Buffer*>(dstBuffer.get());

        m_commandList->CopyBufferRegion(
            d3d12DstBuffer->GetHandle(), dstOffset,
            d3d12SrcBuffer->GetHandle(), srcOffset,
            size);
    }
}
#endif
