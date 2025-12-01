#ifndef CACAO_CACAOPIPELINE_H
#define CACAO_CACAOPIPELINE_H
#include "PipelineDefs.h"
#include "CacaoShaderModule.h"
#include <memory>
#include <vector>
namespace Cacao
{
    class CacaoPipelineLayout;
    class CacaoPipelineCache; 
    class CACAO_API CacaoPipelineCache : public std::enable_shared_from_this<CacaoPipelineCache>
    {
    public:
        virtual ~CacaoPipelineCache() = default;
        virtual std::vector<uint8_t> GetData() const = 0;
        virtual void Merge(const std::vector<Ref<CacaoPipelineCache>>& srcCaches) = 0;
    };
    struct MultisampleState
    {
        uint32_t RasterizationSamples = 1; 
        bool SampleShadingEnable = false;
        float MinSampleShading = 0.0f;
        std::vector<uint32_t> SampleMask; 
        bool AlphaToCoverageEnable = false;
        bool AlphaToOneEnable = false;
    };
    struct GraphicsPipelineCreateInfo
    {
        std::vector<Ref<CacaoShaderModule>> Shaders; 
        std::vector<VertexInputBinding> VertexBindings;
        std::vector<VertexInputAttribute> VertexAttributes;
        InputAssemblyState InputAssembly;
        RasterizationState Rasterizer;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        MultisampleState Multisample; 
        std::vector<Format> ColorAttachmentFormats;
        Format DepthStencilFormat = Format::UNDEFINED;
        Ref<CacaoPipelineLayout> Layout;
        Ref<CacaoPipelineCache> Cache = nullptr;
    };
    class CACAO_API CacaoGraphicsPipeline : public std::enable_shared_from_this<CacaoGraphicsPipeline>
    {
    public:
        virtual ~CacaoGraphicsPipeline() = default;
        virtual Ref<CacaoPipelineLayout> GetLayout() const = 0;
    };
    struct ComputePipelineCreateInfo
    {
        Ref<CacaoShaderModule> ComputeShader;
        Ref<CacaoPipelineLayout> Layout;
        Ref<CacaoPipelineCache> Cache = nullptr;
    };
    class CACAO_API CacaoComputePipeline : public std::enable_shared_from_this<CacaoComputePipeline>
    {
    public:
        virtual ~CacaoComputePipeline() = default;
        virtual Ref<CacaoPipelineLayout> GetLayout() const = 0;
    };
}
#endif 
