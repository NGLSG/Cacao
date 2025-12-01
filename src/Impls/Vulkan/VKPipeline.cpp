#include "Impls/Vulkan/VKPipeline.h"
#include "Impls/Vulkan/VKConvert.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKPipelineLayout.h"
#include "Impls/Vulkan/VKShaderModule.h"
namespace Cacao
{
    inline vk::ShaderStageFlagBits ConvertShaderStage(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
        case ShaderStage::Geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderStage::TessellationControl: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderStage::TessellationEvaluation: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        default:
            throw std::runtime_error("Unknown ShaderStage");
        }
    }
    inline static std::vector<vk::PipelineShaderStageCreateInfo> ConvertShaderStages(
        const std::vector<Ref<CacaoShaderModule>>& shaderModules)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(shaderModules.size());
        for (size_t i = 0; i < shaderModules.size(); i++)
        {
            if (!shaderModules[i])
            {
                throw std::runtime_error("Null shader module in shader stages");
            }
            Ref<VKShaderModule> vkShaderModule = std::dynamic_pointer_cast<VKShaderModule>(shaderModules[i]);
            shaderStages[i].stage = ConvertShaderStage(vkShaderModule->GetStage());
            shaderStages[i].module = vkShaderModule->GetHandle();
            shaderStages[i].pName = "main";
        }
        return shaderStages;
    }
    static vk::PipelineVertexInputStateCreateInfo ConvertVertexInputState(
        const std::vector<VertexInputBinding>& bindings,
        const std::vector<VertexInputAttribute>& attributes)
    {
        std::vector<vk::VertexInputBindingDescription> vkBindings(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
        {
            vkBindings[i].binding = bindings[i].Binding;
            vkBindings[i].stride = bindings[i].Stride;
            vkBindings[i].inputRate = (bindings[i].InputRate == VertexInputRate::Vertex)
                                          ? vk::VertexInputRate::eVertex
                                          : vk::VertexInputRate::eInstance;
        }
        std::vector<vk::VertexInputAttributeDescription> vkAttributes(attributes.size());
        for (size_t i = 0; i < attributes.size(); i++)
        {
            vkAttributes[i].location = attributes[i].Location;
            vkAttributes[i].binding = attributes[i].Binding;
            vkAttributes[i].format = Converter::Convert(attributes[i].Format);
            vkAttributes[i].offset = attributes[i].Offset;
        }
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vkBindings.size());
        vertexInputInfo.pVertexBindingDescriptions = vkBindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = vkAttributes.data();
        return vertexInputInfo;
    }
    static vk::PrimitiveTopology ConvertPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList: return vk::PrimitiveTopology::ePointList;
        case PrimitiveTopology::LineList: return vk::PrimitiveTopology::eLineList;
        case PrimitiveTopology::LineStrip: return vk::PrimitiveTopology::eLineStrip;
        case PrimitiveTopology::TriangleList: return vk::PrimitiveTopology::eTriangleList;
        case PrimitiveTopology::TriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
        case PrimitiveTopology::TriangleFan: return vk::PrimitiveTopology::eTriangleFan;
        case PrimitiveTopology::PatchList: return vk::PrimitiveTopology::ePatchList;
        default:
            throw std::runtime_error("Unknown PrimitiveTopology");
        }
    }
    static vk::CullModeFlags ConvertCullMode(CullMode cullMode)
    {
        switch (cullMode)
        {
        case CullMode::None: return vk::CullModeFlagBits::eNone;
        case CullMode::Front: return vk::CullModeFlagBits::eFront;
        case CullMode::Back: return vk::CullModeFlagBits::eBack;
        case CullMode::FrontAndBack: return vk::CullModeFlagBits::eFrontAndBack;
        default:
            throw std::runtime_error("Unknown CullMode");
        }
    }
    static vk::FrontFace ConvertFrontFace(FrontFace frontFace)
    {
        switch (frontFace)
        {
        case FrontFace::CounterClockwise: return vk::FrontFace::eCounterClockwise;
        case FrontFace::Clockwise: return vk::FrontFace::eClockwise;
        default:
            throw std::runtime_error("Unknown FrontFace");
        }
    }
    static vk::PolygonMode ConvertPolygonMode(PolygonMode polygonMode)
    {
        switch (polygonMode)
        {
        case PolygonMode::Fill: return vk::PolygonMode::eFill;
        case PolygonMode::Line: return vk::PolygonMode::eLine;
        case PolygonMode::Point: return vk::PolygonMode::ePoint;
        default:
            throw std::runtime_error("Unknown PolygonMode");
        }
    }
    static vk::LogicOp ConvertLogicOp(LogicOp logicOp)
    {
        switch (logicOp)
        {
        case LogicOp::Clear: return vk::LogicOp::eClear;
        case LogicOp::And: return vk::LogicOp::eAnd;
        case LogicOp::AndReverse: return vk::LogicOp::eAndReverse;
        case LogicOp::Copy: return vk::LogicOp::eCopy;
        case LogicOp::AndInverted: return vk::LogicOp::eAndInverted;
        case LogicOp::NoOp: return vk::LogicOp::eNoOp;
        case LogicOp::Xor: return vk::LogicOp::eXor;
        case LogicOp::Or: return vk::LogicOp::eOr;
        case LogicOp::Nor: return vk::LogicOp::eNor;
        case LogicOp::Equiv: return vk::LogicOp::eEquivalent;
        case LogicOp::Invert: return vk::LogicOp::eInvert;
        case LogicOp::OrReverse: return vk::LogicOp::eOrReverse;
        case LogicOp::CopyInverted: return vk::LogicOp::eCopyInverted;
        case LogicOp::OrInverted: return vk::LogicOp::eOrInverted;
        case LogicOp::Nand: return vk::LogicOp::eNand;
        case LogicOp::Set: return vk::LogicOp::eSet;
        default:
            throw std::runtime_error("Unknown LogicOp");
        }
    }
    static vk::BlendFactor ConvertBlendFactor(BlendFactor blendFactor)
    {
        switch (blendFactor)
        {
        case BlendFactor::Zero: return vk::BlendFactor::eZero;
        case BlendFactor::One: return vk::BlendFactor::eOne;
        case BlendFactor::SrcColor: return vk::BlendFactor::eSrcColor;
        case BlendFactor::OneMinusSrcColor: return vk::BlendFactor::eOneMinusSrcColor;
        case BlendFactor::DstColor: return vk::BlendFactor::eDstColor;
        case BlendFactor::OneMinusDstColor: return vk::BlendFactor::eOneMinusDstColor;
        case BlendFactor::SrcAlpha: return vk::BlendFactor::eSrcAlpha;
        case BlendFactor::OneMinusSrcAlpha: return vk::BlendFactor::eOneMinusSrcAlpha;
        case BlendFactor::DstAlpha: return vk::BlendFactor::eDstAlpha;
        case BlendFactor::OneMinusDstAlpha: return vk::BlendFactor::eOneMinusDstAlpha;
        case BlendFactor::ConstantColor: return vk::BlendFactor::eConstantColor;
        case BlendFactor::OneMinusConstantColor: return vk::BlendFactor::eOneMinusConstantColor;
        case BlendFactor::SrcAlphaSaturate: return vk::BlendFactor::eSrcAlphaSaturate;
        default:
            throw std::runtime_error("Unknown BlendFactor");
        }
    }
    static vk::PipelineInputAssemblyStateCreateInfo ConvertInputAssemblyState(
        const InputAssemblyState& inputAssembly)
    {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
        inputAssemblyInfo.topology = ConvertPrimitiveTopology(inputAssembly.Topology);
        inputAssemblyInfo.primitiveRestartEnable = inputAssembly.PrimitiveRestartEnable;
        return inputAssemblyInfo;
    }
    static vk::PipelineRasterizationStateCreateInfo ConvertRasterizationState(
        const RasterizationState& rasterizer)
    {
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo = {};
        rasterizerInfo.depthClampEnable = rasterizer.DepthClampEnable;
        rasterizerInfo.rasterizerDiscardEnable = rasterizer.RasterizerDiscardEnable;
        rasterizerInfo.polygonMode = ConvertPolygonMode(rasterizer.PolygonMode);
        rasterizerInfo.cullMode = ConvertCullMode(rasterizer.CullMode);
        rasterizerInfo.frontFace = ConvertFrontFace(rasterizer.FrontFace);
        rasterizerInfo.depthBiasEnable = rasterizer.DepthBiasEnable;
        rasterizerInfo.depthBiasConstantFactor = rasterizer.DepthBiasConstantFactor;
        rasterizerInfo.depthBiasClamp = rasterizer.DepthBiasClamp;
        rasterizerInfo.depthBiasSlopeFactor = rasterizer.DepthBiasSlopeFactor;
        rasterizerInfo.lineWidth = rasterizer.LineWidth;
        return rasterizerInfo;
    }
    static vk::BlendOp ConvertBlendOp(BlendOp blendOp)
    {
        switch (blendOp)
        {
        case BlendOp::Add: return vk::BlendOp::eAdd;
        case BlendOp::Subtract: return vk::BlendOp::eSubtract;
        case BlendOp::ReverseSubtract: return vk::BlendOp::eReverseSubtract;
        case BlendOp::Min: return vk::BlendOp::eMin;
        case BlendOp::Max: return vk::BlendOp::eMax;
        default:
            throw std::runtime_error("Unknown BlendOp");
        }
    }
    static vk::ColorComponentFlags ConvertColorComponentFlags(ColorComponentFlags flags)
    {
        vk::ColorComponentFlags vkFlags = {};
        if (flags & ColorComponentFlags::R)
        {
            vkFlags |= vk::ColorComponentFlagBits::eR;
        }
        if (flags & ColorComponentFlags::G)
        {
            vkFlags |= vk::ColorComponentFlagBits::eG;
        }
        if (flags & ColorComponentFlags::B)
        {
            vkFlags |= vk::ColorComponentFlagBits::eB;
        }
        if (flags & ColorComponentFlags::A)
        {
            vkFlags |= vk::ColorComponentFlagBits::eA;
        }
        return vkFlags;
    }
    static vk::PipelineColorBlendAttachmentState ConvertColorBlendAttachmentState(
        const ColorBlendAttachmentState& attachment)
    {
        vk::PipelineColorBlendAttachmentState vkAttachment = {};
        vkAttachment.blendEnable = attachment.BlendEnable;
        vkAttachment.srcColorBlendFactor = ConvertBlendFactor(attachment.SrcColorBlendFactor);
        vkAttachment.dstColorBlendFactor = ConvertBlendFactor(attachment.DstColorBlendFactor);
        vkAttachment.colorBlendOp = ConvertBlendOp(attachment.ColorBlendOp);
        vkAttachment.srcAlphaBlendFactor = ConvertBlendFactor(attachment.SrcAlphaBlendFactor);
        vkAttachment.dstAlphaBlendFactor = ConvertBlendFactor(attachment.DstAlphaBlendFactor);
        vkAttachment.alphaBlendOp = ConvertBlendOp(attachment.AlphaBlendOp);
        vkAttachment.colorWriteMask = ConvertColorComponentFlags(attachment.ColorWriteMask);
        return vkAttachment;
    }
    static vk::CompareOp ConvertCompareOp(CompareOp compareOp)
    {
        switch (compareOp)
        {
        case CompareOp::Never: return vk::CompareOp::eNever;
        case CompareOp::Less: return vk::CompareOp::eLess;
        case CompareOp::Equal: return vk::CompareOp::eEqual;
        case CompareOp::LessOrEqual: return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater: return vk::CompareOp::eGreater;
        case CompareOp::NotEqual: return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::Always: return vk::CompareOp::eAlways;
        default:
            throw std::runtime_error("Unknown CompareOp");
        }
    }
    static vk::StencilOp ConvertStencilOp(StencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case StencilOp::Keep: return vk::StencilOp::eKeep;
        case StencilOp::Zero: return vk::StencilOp::eZero;
        case StencilOp::Replace: return vk::StencilOp::eReplace;
        case StencilOp::IncrementAndClamp: return vk::StencilOp::eIncrementAndClamp;
        case StencilOp::DecrementAndClamp: return vk::StencilOp::eDecrementAndClamp;
        case StencilOp::Invert: return vk::StencilOp::eInvert;
        case StencilOp::IncrementWrap: return vk::StencilOp::eIncrementAndWrap;
        case StencilOp::DecrementWrap: return vk::StencilOp::eDecrementAndWrap;
        default:
            throw std::runtime_error("Unknown StencilOp");
        }
    }
    static vk::StencilOpState ConvertStencilOpState(const StencilOpState& stencilOpState)
    {
        vk::StencilOpState vkStencilOpState = {};
        vkStencilOpState.failOp = ConvertStencilOp(stencilOpState.FailOp);
        vkStencilOpState.passOp = ConvertStencilOp(stencilOpState.PassOp);
        vkStencilOpState.depthFailOp = ConvertStencilOp(stencilOpState.DepthFailOp);
        vkStencilOpState.compareOp = ConvertCompareOp(stencilOpState.CompareOp);
        vkStencilOpState.compareMask = stencilOpState.CompareMask;
        vkStencilOpState.writeMask = stencilOpState.WriteMask;
        vkStencilOpState.reference = stencilOpState.Reference;
        return vkStencilOpState;
    }
    static vk::PipelineDepthStencilStateCreateInfo ConvertDepthStencilState(const DepthStencilState& depthStencil)
    {
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {};
        depthStencilInfo.depthTestEnable = depthStencil.DepthTestEnable;
        depthStencilInfo.depthWriteEnable = depthStencil.DepthWriteEnable;
        depthStencilInfo.depthCompareOp = ConvertCompareOp(depthStencil.DepthCompareOp);
        depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilInfo.stencilTestEnable = depthStencil.StencilTestEnable;
        depthStencilInfo.front = ConvertStencilOpState(depthStencil.Front);
        depthStencilInfo.back = ConvertStencilOpState(depthStencil.Back);
        depthStencilInfo.minDepthBounds = 0.0f;
        depthStencilInfo.maxDepthBounds = 1.0f;
        return depthStencilInfo;
    }
    static vk::SampleCountFlagBits ConvertSampleCount(uint32_t sampleCount)
    {
        switch (sampleCount)
        {
        case 1: return vk::SampleCountFlagBits::e1;
        case 2: return vk::SampleCountFlagBits::e2;
        case 4: return vk::SampleCountFlagBits::e4;
        case 8: return vk::SampleCountFlagBits::e8;
        case 16: return vk::SampleCountFlagBits::e16;
        case 32: return vk::SampleCountFlagBits::e32;
        case 64: return vk::SampleCountFlagBits::e64;
        default:
            throw std::runtime_error("Unsupported sample count");
        }
    }
    static vk::PipelineMultisampleStateCreateInfo ConvertMultisampleState(
        const MultisampleState& multisample,
        std::vector<vk::SampleMask>& outSampleMask)
    {
        vk::PipelineMultisampleStateCreateInfo multisampleInfo = {};
        multisampleInfo.rasterizationSamples = ConvertSampleCount(multisample.RasterizationSamples);
        multisampleInfo.sampleShadingEnable = multisample.SampleShadingEnable;
        multisampleInfo.minSampleShading = multisample.MinSampleShading;
        multisampleInfo.alphaToCoverageEnable = multisample.AlphaToCoverageEnable;
        multisampleInfo.alphaToOneEnable = multisample.AlphaToOneEnable;
        if (!multisample.SampleMask.empty())
        {
            outSampleMask.resize(multisample.SampleMask.size());
            for (size_t i = 0; i < multisample.SampleMask.size(); i++)
            {
                outSampleMask[i] = multisample.SampleMask[i];
            }
            multisampleInfo.pSampleMask = outSampleMask.data();
        }
        else
        {
            multisampleInfo.pSampleMask = nullptr;
        }
        return multisampleInfo;
    }
    static vk::PipelineColorBlendStateCreateInfo ConvertColorBlendState(
        const ColorBlendState& colorBlend,
        std::vector<vk::PipelineColorBlendAttachmentState>& outAttachments)
    {
        outAttachments.resize(colorBlend.Attachments.size());
        for (size_t i = 0; i < colorBlend.Attachments.size(); i++)
        {
            outAttachments[i] = ConvertColorBlendAttachmentState(colorBlend.Attachments[i]);
        }
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo = {};
        colorBlendInfo.logicOpEnable = colorBlend.LogicOpEnable;
        colorBlendInfo.logicOp = ConvertLogicOp(colorBlend.LogicOp);
        colorBlendInfo.attachmentCount = static_cast<uint32_t>(outAttachments.size());
        colorBlendInfo.pAttachments = outAttachments.data();
        colorBlendInfo.blendConstants[0] = colorBlend.BlendConstants[0];
        colorBlendInfo.blendConstants[1] = colorBlend.BlendConstants[1];
        colorBlendInfo.blendConstants[2] = colorBlend.BlendConstants[2];
        colorBlendInfo.blendConstants[3] = colorBlend.BlendConstants[3];
        return colorBlendInfo;
    }
    static vk::PipelineViewportStateCreateInfo ConvertViewportState(
        uint32_t viewportCount,
        uint32_t scissorCount)
    {
        vk::PipelineViewportStateCreateInfo viewportState = {};
        viewportState.viewportCount = viewportCount;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = scissorCount;
        viewportState.pScissors = nullptr;
        return viewportState;
    }
    static vk::PipelineDynamicStateCreateInfo ConvertDynamicState(
        const std::vector<vk::DynamicState>& dynamicStates)
    {
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {};
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
        return dynamicStateInfo;
    }
    static std::vector<vk::Format> ConvertColorAttachmentFormats(const std::vector<Format>& formats)
    {
        std::vector<vk::Format> vkFormats(formats.size());
        for (size_t i = 0; i < formats.size(); i++)
        {
            vkFormats[i] = Converter::Convert(formats[i]);
        }
        return vkFormats;
    }
    static vk::PipelineRenderingCreateInfo ConvertPipelineRenderingCreateInfo(
        const std::vector<vk::Format>& colorAttachmentFormats,
        vk::Format depthStencilFormat)
    {
        vk::PipelineRenderingCreateInfo renderingInfo = {};
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
        renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        if (depthStencilFormat != vk::Format::eUndefined)
        {
            bool hasDepth = (depthStencilFormat == vk::Format::eD16Unorm ||
                depthStencilFormat == vk::Format::eD32Sfloat ||
                depthStencilFormat == vk::Format::eD16UnormS8Uint ||
                depthStencilFormat == vk::Format::eD24UnormS8Uint ||
                depthStencilFormat == vk::Format::eD32SfloatS8Uint);
            bool hasStencil = (depthStencilFormat == vk::Format::eS8Uint ||
                depthStencilFormat == vk::Format::eD16UnormS8Uint ||
                depthStencilFormat == vk::Format::eD24UnormS8Uint ||
                depthStencilFormat == vk::Format::eD32SfloatS8Uint);
            renderingInfo.depthAttachmentFormat = hasDepth ? depthStencilFormat : vk::Format::eUndefined;
            renderingInfo.stencilAttachmentFormat = hasStencil ? depthStencilFormat : vk::Format::eUndefined;
        }
        else
        {
            renderingInfo.depthAttachmentFormat = vk::Format::eUndefined;
            renderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;
        }
        return renderingInfo;
    }
    VKPipelineCache::VKPipelineCache(const Ref<CacaoDevice>& device,
                                     const std::vector<uint8_t>& initialData)
    {
        if (!device)
        {
            throw std::runtime_error("VKPipelineCache created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
        if (!m_device)
        {
            throw std::runtime_error("VKPipelineCache requires a VKDevice");
        }
        vk::PipelineCacheCreateInfo createInfo = {};
        createInfo.initialDataSize = initialData.size();
        createInfo.pInitialData = initialData.empty() ? nullptr : initialData.data();
        m_pipelineCache = m_device->GetHandle().createPipelineCache(createInfo);
    }
    VKPipelineCache::~VKPipelineCache()
    {
        if (m_pipelineCache && m_device)
        {
            m_device->GetHandle().destroyPipelineCache(m_pipelineCache);
            m_pipelineCache = VK_NULL_HANDLE;
        }
    }
    Ref<VKPipelineCache> VKPipelineCache::Create(const Ref<CacaoDevice>& device,
                                                 const std::vector<uint8_t>& initialData)
    {
        return std::make_shared<VKPipelineCache>(device, initialData);
    }
    std::vector<uint8_t> VKPipelineCache::GetData() const
    {
        if (!m_pipelineCache || !m_device)
        {
            return {};
        }
        size_t dataSize = 0;
        vk::Result result = m_device->GetHandle().getPipelineCacheData(m_pipelineCache, &dataSize, nullptr);
        if (result != vk::Result::eSuccess || dataSize == 0)
        {
            return {};
        }
        std::vector<uint8_t> data(dataSize);
        result = m_device->GetHandle().getPipelineCacheData(m_pipelineCache, &dataSize, data.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to get pipeline cache data: " + vk::to_string(result));
        }
        data.resize(dataSize);
        return data;
    }
    void VKPipelineCache::Merge(const std::vector<Ref<CacaoPipelineCache>>& srcCaches)
    {
        if (!m_pipelineCache || !m_device)
        {
            throw std::runtime_error("Cannot merge into invalid pipeline cache");
        }
        if (srcCaches.empty())
        {
            return;
        }
        std::vector<vk::PipelineCache> vkSrcCaches;
        vkSrcCaches.reserve(srcCaches.size());
        for (const auto& srcCache : srcCaches)
        {
            if (!srcCache)
            {
                continue;
            }
            Ref<VKPipelineCache> vkCache = std::dynamic_pointer_cast<VKPipelineCache>(srcCache);
            if (!vkCache)
            {
                throw std::runtime_error("Cannot merge non-Vulkan pipeline cache into VKPipelineCache");
            }
            if (vkCache->GetHandle())
            {
                vkSrcCaches.push_back(vkCache->GetHandle());
            }
        }
        if (vkSrcCaches.empty())
        {
            return;
        }
        vk::Result result = m_device->GetHandle().mergePipelineCaches(
            m_pipelineCache,
            static_cast<uint32_t>(vkSrcCaches.size()),
            vkSrcCaches.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to merge pipeline caches: " + vk::to_string(result));
        }
    }
    static vk::PipelineVertexInputStateCreateInfo ConvertVertexInputState(
        const std::vector<VertexInputBinding>& bindings,
        const std::vector<VertexInputAttribute>& attributes,
        std::vector<vk::VertexInputBindingDescription>& outBindings,
        std::vector<vk::VertexInputAttributeDescription>& outAttributes)
    {
        outBindings.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
        {
            outBindings[i].binding = bindings[i].Binding;
            outBindings[i].stride = bindings[i].Stride;
            outBindings[i].inputRate = (bindings[i].InputRate == VertexInputRate::Vertex)
                                           ? vk::VertexInputRate::eVertex
                                           : vk::VertexInputRate::eInstance;
        }
        outAttributes.resize(attributes.size());
        for (size_t i = 0; i < attributes.size(); i++)
        {
            outAttributes[i].location = attributes[i].Location;
            outAttributes[i].binding = attributes[i].Binding;
            outAttributes[i].format = Converter::Convert(attributes[i].Format);
            outAttributes[i].offset = attributes[i].Offset;
        }
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(outBindings.size());
        vertexInputInfo.pVertexBindingDescriptions = outBindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(outAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = outAttributes.data();
        return vertexInputInfo;
    }
    VKGraphicsPipeline::VKGraphicsPipeline(const Ref<CacaoDevice>& device, const GraphicsPipelineCreateInfo& createInfo)
    {
        if (!device)
        {
            throw std::runtime_error("VKGraphicsPipeline created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
        if (!m_device)
        {
            throw std::runtime_error("VKGraphicsPipeline requires a VKDevice");
        }
        m_pipelineInfo = createInfo;
        vk::GraphicsPipelineCreateInfo pipelineInfo = {};
        m_shaderStages = ConvertShaderStages(createInfo.Shaders);
        pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
        pipelineInfo.pStages = m_shaderStages.data();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = ConvertVertexInputState(
            createInfo.VertexBindings,
            createInfo.VertexAttributes,
            m_vertexBindings,
            m_vertexAttributes);
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
            ConvertInputAssemblyState(createInfo.InputAssembly);
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        vk::PipelineTessellationStateCreateInfo tessellationInfo{};
        if (createInfo.InputAssembly.Topology == PrimitiveTopology::PatchList)
        {
            tessellationInfo.patchControlPoints = createInfo.InputAssembly.PatchControlPoints;
            pipelineInfo.pTessellationState = &tessellationInfo;
        }
        else
        {
            pipelineInfo.pTessellationState = nullptr;
        }
        vk::PipelineViewportStateCreateInfo viewportState = {};
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;
        pipelineInfo.pViewportState = &viewportState;
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo =
            ConvertRasterizationState(createInfo.Rasterizer);
        pipelineInfo.pRasterizationState = &rasterizerInfo;
        vk::PipelineMultisampleStateCreateInfo multisampleInfo = ConvertMultisampleState(
            createInfo.Multisample,
            m_sampleMask);
        pipelineInfo.pMultisampleState = &multisampleInfo;
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo =
            ConvertDepthStencilState(createInfo.DepthStencil);
        pipelineInfo.pDepthStencilState = &depthStencilInfo;
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo = ConvertColorBlendState(
            createInfo.ColorBlend,
            m_colorBlendAttachments);
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        m_dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {};
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
        dynamicStateInfo.pDynamicStates = m_dynamicStates.data();
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        if (!createInfo.Layout)
        {
            throw std::runtime_error("VKGraphicsPipeline requires a valid PipelineLayout");
        }
        Ref<VKPipelineLayout> vkLayout = std::dynamic_pointer_cast<VKPipelineLayout>(createInfo.Layout);
        if (!vkLayout)
        {
            throw std::runtime_error("VKGraphicsPipeline requires a VKPipelineLayout");
        }
        pipelineInfo.layout = vkLayout->GetHandle();
        m_colorAttachmentFormats = ConvertColorAttachmentFormats(createInfo.ColorAttachmentFormats);
        vk::Format depthStencilVkFormat = (createInfo.DepthStencilFormat != Format::UNDEFINED)
                                              ? Converter::Convert(createInfo.DepthStencilFormat)
                                              : vk::Format::eUndefined;
        vk::PipelineRenderingCreateInfo renderingInfo = ConvertPipelineRenderingCreateInfo(
            m_colorAttachmentFormats,
            depthStencilVkFormat);
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        vk::PipelineCache pipelineCache = VK_NULL_HANDLE;
        if (createInfo.Cache)
        {
            Ref<VKPipelineCache> vkCache = std::dynamic_pointer_cast<VKPipelineCache>(createInfo.Cache);
            if (vkCache)
            {
                pipelineCache = vkCache->GetHandle();
            }
        }
        auto result = m_device->GetHandle().createGraphicsPipeline(pipelineCache, pipelineInfo);
        if (result.result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create graphics pipeline: " +
                vk::to_string(result.result));
        }
        m_pipeline = result.value;
    }
    VKGraphicsPipeline::~VKGraphicsPipeline()
    {
        if (m_pipeline && m_device)
        {
            m_device->GetHandle().destroyPipeline(m_pipeline);
            m_pipeline = VK_NULL_HANDLE;
        }
    }
    Ref<VKGraphicsPipeline> VKGraphicsPipeline::Create(const Ref<CacaoDevice>& device,
                                                       const GraphicsPipelineCreateInfo& createInfo)
    {
        return std::make_shared<VKGraphicsPipeline>(device, createInfo);
    }
    Ref<CacaoPipelineLayout> VKGraphicsPipeline::GetLayout() const
    {
        return m_pipelineInfo.Layout;
    }
    VKComputePipeline::VKComputePipeline(const Ref<CacaoDevice>& device,
                                         const ComputePipelineCreateInfo& createInfo)
    {
        if (!device)
        {
            throw std::runtime_error("VKComputePipeline created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
        if (!m_device)
        {
            throw std::runtime_error("VKComputePipeline requires a VKDevice");
        }
        m_pipelineInfo = createInfo;
        if (!createInfo.ComputeShader)
        {
            throw std::runtime_error("VKComputePipeline requires a valid compute shader");
        }
        Ref<VKShaderModule> vkShaderModule = std::dynamic_pointer_cast<VKShaderModule>(createInfo.ComputeShader);
        if (!vkShaderModule)
        {
            throw std::runtime_error("VKComputePipeline requires a VKShaderModule");
        }
        if (vkShaderModule->GetStage() != ShaderStage::Compute)
        {
            throw std::runtime_error("VKComputePipeline requires a compute shader, not " +
                std::to_string(static_cast<int>(vkShaderModule->GetStage())));
        }
        if (!createInfo.Layout)
        {
            throw std::runtime_error("VKComputePipeline requires a valid PipelineLayout");
        }
        Ref<VKPipelineLayout> vkLayout = std::dynamic_pointer_cast<VKPipelineLayout>(createInfo.Layout);
        if (!vkLayout)
        {
            throw std::runtime_error("VKComputePipeline requires a VKPipelineLayout");
        }
        vk::ComputePipelineCreateInfo pipelineInfo = {};
        vk::PipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageInfo.module = vkShaderModule->GetHandle();
        shaderStageInfo.pName = vkShaderModule->GetEntryPoint().c_str();
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = vkLayout->GetHandle();
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        vk::PipelineCache pipelineCache = VK_NULL_HANDLE;
        if (createInfo.Cache)
        {
            Ref<VKPipelineCache> vkCache = std::dynamic_pointer_cast<VKPipelineCache>(createInfo.Cache);
            if (vkCache)
            {
                pipelineCache = vkCache->GetHandle();
            }
        }
        auto result = m_device->GetHandle().createComputePipeline(pipelineCache, pipelineInfo);
        if (result.result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create compute pipeline: " +
                vk::to_string(result.result));
        }
        m_pipeline = result.value;
    }
    VKComputePipeline::~VKComputePipeline()
    {
        if (m_pipeline && m_device)
        {
            m_device->GetHandle().destroyPipeline(m_pipeline);
            m_pipeline = VK_NULL_HANDLE;
        }
    }
    Ref<VKComputePipeline> VKComputePipeline::Create(const Ref<CacaoDevice>& device,
                                                     const ComputePipelineCreateInfo& createInfo)
    {
        return std::make_shared<VKComputePipeline>(device, createInfo);
    }
    Ref<CacaoPipelineLayout> VKComputePipeline::GetLayout() const
    {
        return m_pipelineInfo.Layout;
    }
}
