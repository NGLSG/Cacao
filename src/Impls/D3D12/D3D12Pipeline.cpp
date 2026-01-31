#ifdef WIN32
#include "Impls/D3D12/D3D12Pipeline.h"
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12PipelineLayout.h"
#include "Impls/D3D12/D3D12ShaderModule.h"
#include <stdexcept>

namespace Cacao
{
    // ========================= D3D12PipelineCache =========================

    D3D12PipelineCache::D3D12PipelineCache(const Ref<Device>& device, std::span<const uint8_t> initialData)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12PipelineCache created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12PipelineCache requires a D3D12Device");
        }

        // D3D12 使用 Pipeline Library 来缓存 PSO
        // 如果有初始数据，尝试从中加载
        if (!initialData.empty())
        {
            m_cacheData.assign(initialData.begin(), initialData.end());

            HRESULT hr = m_device->GetHandle()->CreatePipelineLibrary(
                m_cacheData.data(),
                m_cacheData.size(),
                IID_PPV_ARGS(&m_pipelineLibrary));

            if (FAILED(hr))
            {
                // 如果加载失败，创建一个新的空库
                m_cacheData.clear();
                hr = m_device->GetHandle()->CreatePipelineLibrary(
                    nullptr, 0, IID_PPV_ARGS(&m_pipelineLibrary));

                if (FAILED(hr))
                {
                    throw std::runtime_error("Failed to create D3D12 pipeline library");
                }
            }
        }
        else
        {
            HRESULT hr = m_device->GetHandle()->CreatePipelineLibrary(
                nullptr, 0, IID_PPV_ARGS(&m_pipelineLibrary));

            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to create D3D12 pipeline library");
            }
        }
    }

    D3D12PipelineCache::~D3D12PipelineCache()
    {
        m_pipelineLibrary.Reset();
    }

    std::vector<uint8_t> D3D12PipelineCache::GetData() const
    {
        if (!m_pipelineLibrary)
        {
            return {};
        }

        SIZE_T size = m_pipelineLibrary->GetSerializedSize();
        if (size == 0)
        {
            return {};
        }

        std::vector<uint8_t> data(size);
        HRESULT hr = m_pipelineLibrary->Serialize(data.data(), size);
        if (FAILED(hr))
        {
            return {};
        }

        return data;
    }

    void D3D12PipelineCache::Merge(std::span<const Ref<PipelineCache>> srcCaches)
    {
        // D3D12 Pipeline Library 不直接支持合并
        // 这里我们只是记录一个警告，实际使用中可能需要重新创建 PSO
        (void)srcCaches;
    }

    Ref<PipelineCache> D3D12PipelineCache::Create(const Ref<Device>& device, std::span<const uint8_t> initialData)
    {
        return CreateRef<D3D12PipelineCache>(device, initialData);
    }

    // ========================= D3D12GraphicsPipeline =========================

    static D3D12_RENDER_TARGET_BLEND_DESC ConvertBlendState(const ColorBlendAttachmentState& state)
    {
        D3D12_RENDER_TARGET_BLEND_DESC desc = {};
        desc.BlendEnable = state.BlendEnable;
        desc.LogicOpEnable = FALSE;
        desc.SrcBlend = D3D12Converter::Convert(state.SrcColorBlendFactor);
        desc.DestBlend = D3D12Converter::Convert(state.DstColorBlendFactor);
        desc.BlendOp = D3D12Converter::Convert(state.ColorBlendOp);
        desc.SrcBlendAlpha = D3D12Converter::Convert(state.SrcAlphaBlendFactor);
        desc.DestBlendAlpha = D3D12Converter::Convert(state.DstAlphaBlendFactor);
        desc.BlendOpAlpha = D3D12Converter::Convert(state.AlphaBlendOp);
        desc.LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.RenderTargetWriteMask = D3D12Converter::Convert(state.ColorWriteMask);
        return desc;
    }

    static D3D12_DEPTH_STENCILOP_DESC ConvertStencilOpState(const StencilOpState& state)
    {
        D3D12_DEPTH_STENCILOP_DESC desc = {};
        desc.StencilFailOp = D3D12Converter::Convert(state.FailOp);
        desc.StencilDepthFailOp = D3D12Converter::Convert(state.DepthFailOp);
        desc.StencilPassOp = D3D12Converter::Convert(state.PassOp);
        desc.StencilFunc = D3D12Converter::Convert(state.CompareOp);
        return desc;
    }

    D3D12GraphicsPipeline::D3D12GraphicsPipeline(const Ref<Device>& device, const GraphicsPipelineCreateInfo& info)
        : m_createInfo(info)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12GraphicsPipeline created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12GraphicsPipeline requires a D3D12Device");
        }

        if (!info.Layout)
        {
            throw std::runtime_error("D3D12GraphicsPipeline requires a valid PipelineLayout");
        }

        auto* d3d12Layout = dynamic_cast<D3D12PipelineLayout*>(info.Layout.get());
        if (!d3d12Layout)
        {
            throw std::runtime_error("D3D12GraphicsPipeline requires a D3D12PipelineLayout");
        }

        // 保存图元拓扑（用于绘制时设置）
        m_primitiveTopology = D3D12Converter::Convert(info.InputAssembly.Topology);

        // 使用 D3D12_GRAPHICS_PIPELINE_STATE_DESC 创建 PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = d3d12Layout->GetRootSignature().Get();

        // 设置着色器
        for (const auto& shader : info.Shaders)
        {
            if (!shader)
            {
                continue;
            }

            auto* d3d12Shader = dynamic_cast<D3D12ShaderModule*>(shader.get());
            if (!d3d12Shader)
            {
                throw std::runtime_error("D3D12GraphicsPipeline requires D3D12ShaderModule");
            }

            D3D12_SHADER_BYTECODE bytecode = d3d12Shader->GetBytecode();

            switch (d3d12Shader->GetStage())
            {
            case ShaderStage::Vertex:
                psoDesc.VS = bytecode;
                break;
            case ShaderStage::Fragment:
                psoDesc.PS = bytecode;
                break;
            case ShaderStage::Geometry:
                psoDesc.GS = bytecode;
                break;
            case ShaderStage::TessellationControl:
                psoDesc.HS = bytecode;
                break;
            case ShaderStage::TessellationEvaluation:
                psoDesc.DS = bytecode;
                break;
            default:
                break;
            }
        }

        // 设置输入布局
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
        for (const auto& attr : info.VertexAttributes)
        {
            D3D12_INPUT_ELEMENT_DESC element = {};
            element.SemanticName = "TEXCOORD"; // 使用通用语义
            element.SemanticIndex = attr.Location;
            element.Format = D3D12Converter::Convert(attr.Format);
            element.InputSlot = attr.Binding;
            element.AlignedByteOffset = attr.Offset;

            // 查找对应的绑定来确定输入率
            for (const auto& binding : info.VertexBindings)
            {
                if (binding.Binding == attr.Binding)
                {
                    element.InputSlotClass = (binding.InputRate == VertexInputRate::Vertex)
                                                 ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
                                                 : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                    element.InstanceDataStepRate = (binding.InputRate == VertexInputRate::Instance) ? 1 : 0;
                    break;
                }
            }

            inputElements.push_back(element);
        }

        psoDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());
        psoDesc.InputLayout.pInputElementDescs = inputElements.data();

        // 设置光栅化状态
        psoDesc.RasterizerState.FillMode = D3D12Converter::Convert(info.Rasterizer.PolygonMode);
        psoDesc.RasterizerState.CullMode = D3D12Converter::Convert(info.Rasterizer.CullMode);
        psoDesc.RasterizerState.FrontCounterClockwise = D3D12Converter::ConvertFrontFace(info.Rasterizer.FrontFace);
        psoDesc.RasterizerState.DepthBias = static_cast<INT>(info.Rasterizer.DepthBiasConstantFactor);
        psoDesc.RasterizerState.DepthBiasClamp = info.Rasterizer.DepthBiasClamp;
        psoDesc.RasterizerState.SlopeScaledDepthBias = info.Rasterizer.DepthBiasSlopeFactor;
        psoDesc.RasterizerState.DepthClipEnable = !info.Rasterizer.DepthClampEnable;
        psoDesc.RasterizerState.MultisampleEnable = info.Multisample.RasterizationSamples > 1;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // 设置混合状态
        psoDesc.BlendState.AlphaToCoverageEnable = info.Multisample.AlphaToCoverageEnable;
        psoDesc.BlendState.IndependentBlendEnable = TRUE;

        for (size_t i = 0; i < info.ColorBlend.Attachments.size() && i < 8; ++i)
        {
            psoDesc.BlendState.RenderTarget[i] = ConvertBlendState(info.ColorBlend.Attachments[i]);

            if (info.ColorBlend.LogicOpEnable)
            {
                psoDesc.BlendState.RenderTarget[i].LogicOpEnable = TRUE;
                psoDesc.BlendState.RenderTarget[i].LogicOp = D3D12Converter::Convert(info.ColorBlend.LogicOp);
            }
        }

        // 设置深度模板状态
        psoDesc.DepthStencilState.DepthEnable = info.DepthStencil.DepthTestEnable;
        psoDesc.DepthStencilState.DepthWriteMask = info.DepthStencil.DepthWriteEnable
                                                       ? D3D12_DEPTH_WRITE_MASK_ALL
                                                       : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12Converter::Convert(info.DepthStencil.DepthCompareOp);
        psoDesc.DepthStencilState.StencilEnable = info.DepthStencil.StencilTestEnable;
        psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(info.DepthStencil.StencilReadMask);
        psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(info.DepthStencil.StencilWriteMask);
        psoDesc.DepthStencilState.FrontFace = ConvertStencilOpState(info.DepthStencil.Front);
        psoDesc.DepthStencilState.BackFace = ConvertStencilOpState(info.DepthStencil.Back);

        // 设置图元拓扑类型
        psoDesc.PrimitiveTopologyType = D3D12Converter::ConvertTopologyType(info.InputAssembly.Topology);

        // 设置渲染目标格式
        psoDesc.NumRenderTargets = static_cast<UINT>(info.ColorAttachmentFormats.size());
        for (size_t i = 0; i < info.ColorAttachmentFormats.size() && i < 8; ++i)
        {
            psoDesc.RTVFormats[i] = D3D12Converter::Convert(info.ColorAttachmentFormats[i]);
        }

        // 设置深度模板格式
        if (info.DepthStencilFormat != Format::UNDEFINED)
        {
            psoDesc.DSVFormat = D3D12Converter::Convert(info.DepthStencilFormat);
        }
        else
        {
            psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        }

        // 设置多重采样
        psoDesc.SampleDesc.Count = info.Multisample.RasterizationSamples;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.SampleMask = UINT_MAX;

        // 其他设置
        psoDesc.IBStripCutValue = info.InputAssembly.PrimitiveRestartEnable
                                      ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF
                                      : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        psoDesc.NodeMask = 0;
        psoDesc.CachedPSO = {};
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        // 创建 PSO
        HRESULT hr = m_device->GetHandle()->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&m_pipelineState));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 graphics pipeline state: HRESULT " + std::to_string(hr));
        }
    }

    D3D12GraphicsPipeline::~D3D12GraphicsPipeline()
    {
        m_pipelineState.Reset();
    }

    Ref<PipelineLayout> D3D12GraphicsPipeline::GetLayout() const
    {
        return m_createInfo.Layout;
    }

    Ref<GraphicsPipeline> D3D12GraphicsPipeline::Create(const Ref<Device>& device,
                                                        const GraphicsPipelineCreateInfo& info)
    {
        return CreateRef<D3D12GraphicsPipeline>(device, info);
    }

    // ========================= D3D12ComputePipeline =========================

    D3D12ComputePipeline::D3D12ComputePipeline(const Ref<Device>& device, const ComputePipelineCreateInfo& info)
        : m_createInfo(info)
    {
        if (!device)
        {
            throw std::runtime_error("D3D12ComputePipeline created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a D3D12Device");
        }

        if (!info.ComputeShader)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a valid compute shader");
        }

        if (!info.Layout)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a valid PipelineLayout");
        }

        auto* d3d12Layout = dynamic_cast<D3D12PipelineLayout*>(info.Layout.get());
        if (!d3d12Layout)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a D3D12PipelineLayout");
        }

        auto* d3d12Shader = dynamic_cast<D3D12ShaderModule*>(info.ComputeShader.get());
        if (!d3d12Shader)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a D3D12ShaderModule");
        }

        if (d3d12Shader->GetStage() != ShaderStage::Compute)
        {
            throw std::runtime_error("D3D12ComputePipeline requires a compute shader");
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = d3d12Layout->GetRootSignature().Get();
        psoDesc.CS = d3d12Shader->GetBytecode();
        psoDesc.NodeMask = 0;
        psoDesc.CachedPSO = {};
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        HRESULT hr = m_device->GetHandle()->CreateComputePipelineState(
            &psoDesc, IID_PPV_ARGS(&m_pipelineState));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 compute pipeline state");
        }
    }

    D3D12ComputePipeline::~D3D12ComputePipeline()
    {
        m_pipelineState.Reset();
    }

    Ref<PipelineLayout> D3D12ComputePipeline::GetLayout() const
    {
        return m_createInfo.Layout;
    }

    Ref<ComputePipeline> D3D12ComputePipeline::Create(const Ref<Device>& device, const ComputePipelineCreateInfo& info)
    {
        return CreateRef<D3D12ComputePipeline>(device, info);
    }
}
#endif
