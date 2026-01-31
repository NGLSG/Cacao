#ifdef WIN32
#include "Impls/D3D12/D3D12DescriptorSet.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12DescriptorPool.h"
#include "Impls/D3D12/D3D12DescriptorSetLayout.h"
#include "Impls/D3D12/D3D12Buffer.h"
#include "Impls/D3D12/D3D12Texture.h"
#include "Impls/D3D12/D3D12Sampler.h"
#include <stdexcept>

namespace Cacao
{
    D3D12DescriptorSet::D3D12DescriptorSet(const Ref<D3D12Device>& device,
                                           const Ref<D3D12DescriptorPool>& pool,
                                           const Ref<DescriptorSetLayout>& layout)
        : m_device(device), m_pool(pool)
    {
        m_layout = std::dynamic_pointer_cast<D3D12DescriptorSetLayout>(layout);
        if (!m_layout)
        {
            throw std::runtime_error("D3D12DescriptorSet requires a D3D12DescriptorSetLayout");
        }

        m_cbvSrvUavCount = m_layout->GetCbvSrvUavCount();
        m_samplerCount = m_layout->GetSamplerCount();

        // 从池中分配描述符
        // 注意：这里获得的是连续内存块的起始 CPU 句柄
        if (m_cbvSrvUavCount > 0)
        {
            m_cbvSrvUavGpuHandle = m_pool->AllocateCbvSrvUav(m_cbvSrvUavCount, m_cbvSrvUavCpuHandle);
        }

        if (m_samplerCount > 0)
        {
            m_samplerGpuHandle = m_pool->AllocateSampler(m_samplerCount, m_samplerCpuHandle);
        }
    }

    // 内部辅助函数：根据 Binding 号计算在对应堆中的线性偏移量
    // 必须与 PipelineLayout 创建 RootSignature 时的遍历逻辑保持一致
    uint32_t GetHeapOffset(const D3D12DescriptorSetLayout* layout, uint32_t targetBinding,
                           D3D12_DESCRIPTOR_HEAP_TYPE targetHeapType)
    {
        const auto& bindings = layout->GetBindings();
        uint32_t offset = 0;
        bool found = false;

        for (const auto& binding : bindings)
        {
            // 判断当前 binding 的类型是否属于目标堆 (Sampler vs CBV/SRV/UAV)
            D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12Converter::Convert(binding.Type);
            bool isSampler = (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
            bool targetIsSampler = (targetHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            if (isSampler == targetIsSampler)
            {
                if (binding.Binding == targetBinding)
                {
                    found = true;
                    break; // 找到目标，当前的 offset 即为正确偏移
                }
                // 如果不是目标但属于同类堆，累加偏移
                offset += binding.Count;
            }
        }

        if (!found)
        {
            // 如果没找到，说明 Layout 中没定义这个 Binding，通常应抛出异常或返回错误
            // 这里为了安全返回 0，但在 Debug 模式下应该报错
            return 0;
        }

        return offset;
    }

    void D3D12DescriptorSet::WriteBuffer(const BufferWriteInfo& info)
    {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(info.Buffer.get());

        // 1. 获取正确的 CPU 句柄位置
        uint32_t heapOffset = GetHeapOffset(m_layout.get(), info.Binding, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        UINT descriptorSize = m_device->GetHandle()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cbvSrvUavCpuHandle;
        cpuHandle.ptr += (heapOffset + info.ArrayElement) * descriptorSize;

        // 2. 确定描述符类型
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12Converter::Convert(info.Type);

        // [关键修复]：检查缓冲区内存类型
        // 如果 Buffer 在 Upload 堆 (CpuToGpu)，绝对不能创建 UAV，只能创建 SRV。
        // 即使 DescriptorType 是 StorageBuffer，我们也必须将其降级为 SRV。
        if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
            if (d3d12Buffer->GetMemoryUsage() == BufferMemoryUsage::CpuToGpu)
            {
                // Upload Heap 不支持 UAV，强制转换为 SRV
                rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            }
        }

        if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = d3d12Buffer->GetGPUVirtualAddress() + info.Offset;
            cbvDesc.SizeInBytes = static_cast<UINT>(
                (info.Size == UINT64_MAX) ? d3d12Buffer->GetSize() - info.Offset : info.Size);
            cbvDesc.SizeInBytes = (cbvDesc.SizeInBytes + 255) & ~255;

            m_device->GetHandle()->CreateConstantBufferView(&cbvDesc, cpuHandle);
        }
        else if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured Buffer
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            // 计算 Stride 和 NumElements
            // 注意：由于 BufferWriteInfo 中可能没有 Stride 信息，我们暂时默认为 4 (float)
            // 如果你的 Shader 中定义的是 StructuredBuffer<SpriteInstance>，你可能需要将此处改为 sizeof(SpriteInstance) (64)
            // 或者更新 BufferWriteInfo 结构体以包含 Stride。
            // 这里为了保证程序不崩溃，使用 4，Shader 中需要以 float/float4 形式读取或者依赖 D3D12 的隐式匹配。
            uint32_t stride = info.Stride;

            srvDesc.Buffer.FirstElement = info.Offset / stride;
            srvDesc.Buffer.NumElements = static_cast<UINT>(
                (info.Size == UINT64_MAX) ? (d3d12Buffer->GetSize() - info.Offset) / stride : info.Size / stride);
            srvDesc.Buffer.StructureByteStride = stride;
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            m_device->GetHandle()->CreateShaderResourceView(d3d12Buffer->GetHandle(), &srvDesc, cpuHandle);
        }
        else if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
            // 只有 Default Heap 且带有 AllowUAV 标志的 Buffer 才能到达这里
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

            uint32_t stride = info.Stride;

            uavDesc.Buffer.FirstElement = info.Offset / stride;
            uavDesc.Buffer.NumElements = static_cast<UINT>(
                (info.Size == UINT64_MAX) ? (d3d12Buffer->GetSize() - info.Offset) / stride : info.Size / stride);
            uavDesc.Buffer.StructureByteStride = stride;
            uavDesc.Buffer.CounterOffsetInBytes = 0;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            m_device->GetHandle()->CreateUnorderedAccessView(d3d12Buffer->GetHandle(), nullptr, &uavDesc, cpuHandle);
        }
    }

    void D3D12DescriptorSet::WriteBuffers(const BufferWriteInfos& infos)
    {
        for (size_t i = 0; i < infos.Buffers.size(); ++i)
        {
            BufferWriteInfo info = {};
            info.Binding = infos.Binding;
            info.Buffer = infos.Buffers[i];
            info.Offset = (i < infos.Offsets.size()) ? infos.Offsets[i] : 0;
            info.Size = (i < infos.Sizes.size()) ? infos.Sizes[i] : UINT64_MAX;
            info.Type = infos.Type;
            info.ArrayElement = infos.ArrayElement + static_cast<uint32_t>(i);
            WriteBuffer(info);
        }
    }

    void D3D12DescriptorSet::WriteTexture(const TextureWriteInfo& info)
    {
        // 计算偏移量
        uint32_t heapOffset = GetHeapOffset(m_layout.get(), info.Binding, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        UINT descriptorSize = m_device->GetHandle()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cbvSrvUavCpuHandle;
        cpuHandle.ptr += (heapOffset + info.ArrayElement) * descriptorSize;

        auto* d3d12View = static_cast<D3D12TextureView*>(info.TextureView.get());
        auto* d3d12Texture = static_cast<D3D12Texture*>(d3d12View->GetTexture().get());

        D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12Converter::Convert(info.Type);

        if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = D3D12Converter::Convert(d3d12Texture->GetFormat());
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            switch (d3d12Texture->GetType())
            {
            case TextureType::Texture1D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MipLevels = d3d12Texture->GetMipLevels();
                break;
            case TextureType::Texture2D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = d3d12Texture->GetMipLevels();
                break;
            case TextureType::Texture3D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MipLevels = d3d12Texture->GetMipLevels();
                break;
            case TextureType::TextureCube:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = d3d12Texture->GetMipLevels();
                break;
            case TextureType::Texture2DArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MipLevels = d3d12Texture->GetMipLevels();
                srvDesc.Texture2DArray.ArraySize = d3d12Texture->GetArrayLayers();
                break;
            default:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = d3d12Texture->GetMipLevels();
                break;
            }

            m_device->GetHandle()->CreateShaderResourceView(d3d12Texture->GetResource(), &srvDesc, cpuHandle);
        }
        else if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = D3D12Converter::Convert(d3d12Texture->GetFormat());
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

            m_device->GetHandle()->CreateUnorderedAccessView(d3d12Texture->GetResource(), nullptr, &uavDesc, cpuHandle);
        }
    }

    void D3D12DescriptorSet::WriteTextures(const TextureWriteInfos& infos)
    {
        for (size_t i = 0; i < infos.TextureViews.size(); ++i)
        {
            TextureWriteInfo info = {};
            info.Binding = infos.Binding;
            info.TextureView = infos.TextureViews[i];
            info.Layout = (i < infos.Layouts.size()) ? infos.Layouts[i] : ImageLayout::ShaderReadOnly;
            info.Type = infos.Type;
            info.Sampler = nullptr;
            info.ArrayElement = infos.ArrayElement + static_cast<uint32_t>(i);
            WriteTexture(info);
        }
    }

    void D3D12DescriptorSet::WriteSampler(const SamplerWriteInfo& info)
    {
        auto* d3d12Sampler = static_cast<D3D12Sampler*>(info.Sampler.get());

        // 这里的关键修复：使用 Sampler 堆，并计算 Sampler 堆内的偏移
        uint32_t heapOffset = GetHeapOffset(m_layout.get(), info.Binding, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        UINT descriptorSize = m_device->GetHandle()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_samplerCpuHandle;
        cpuHandle.ptr += (heapOffset + info.ArrayElement) * descriptorSize;

        m_device->GetHandle()->CreateSampler(&d3d12Sampler->GetSamplerDesc(), cpuHandle);
    }

    void D3D12DescriptorSet::WriteSamplers(const SamplerWriteInfos& infos)
    {
        for (size_t i = 0; i < infos.Samplers.size(); ++i)
        {
            SamplerWriteInfo info = {};
            info.Binding = infos.Binding;
            info.Sampler = infos.Samplers[i];
            info.ArrayElement = infos.ArrayElement + static_cast<uint32_t>(i);
            WriteSampler(info);
        }
    }

    void D3D12DescriptorSet::WriteAccelerationStructure(const AccelerationStructureWriteInfo& info)
    {
        // TODO: 实现加速结构写入
    }

    void D3D12DescriptorSet::WriteAccelerationStructures(const AccelerationStructureWriteInfos& infos)
    {
        // TODO: 实现加速结构批量写入
    }

    void D3D12DescriptorSet::Update()
    {
        // D3D12 描述符是立即更新的，不需要额外操作
    }

    Ref<DescriptorSet> D3D12DescriptorSet::Create(const Ref<D3D12Device>& device,
                                                  const Ref<D3D12DescriptorPool>& pool,
                                                  const Ref<DescriptorSetLayout>& layout)
    {
        return CreateRef<D3D12DescriptorSet>(device, pool, layout);
    }
}
#endif
