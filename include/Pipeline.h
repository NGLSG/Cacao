#ifndef CACAO_CACAOPIPELINE_H
#define CACAO_CACAOPIPELINE_H
#include "PipelineDefs.h"
#include "ShaderModule.h"
#include <memory>
#include <vector>
namespace Cacao
{
    class PipelineLayout;
    class CacaoPipelineCache; 
    class CACAO_API CacaoPipelineCache : public std::enable_shared_from_this<CacaoPipelineCache>
    {
    public:
        virtual ~CacaoPipelineCache() = default;
        virtual std::vector<uint8_t> GetData() const = 0;
        virtual void Merge(std::span<const Ref<CacaoPipelineCache>> srcCaches) = 0;
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
        std::vector<Ref<ShaderModule>> Shaders;
        std::vector<VertexInputBinding> VertexBindings;
        std::vector<VertexInputAttribute> VertexAttributes;
        InputAssemblyState InputAssembly;
        RasterizationState Rasterizer;
        DepthStencilState DepthStencil;
        ColorBlendState ColorBlend;
        MultisampleState Multisample; 
        std::vector<Format> ColorAttachmentFormats;
        Format DepthStencilFormat = Format::UNDEFINED;
        Ref<PipelineLayout> Layout;
        Ref<CacaoPipelineCache> Cache = nullptr;
    };
    class CACAO_API GraphicsPipeline : public std::enable_shared_from_this<GraphicsPipeline>
    {
    public:
        virtual ~GraphicsPipeline() = default;
        virtual Ref<PipelineLayout> GetLayout() const = 0;
    };
    struct ComputePipelineCreateInfo
    {
        Ref<ShaderModule> ComputeShader;
        Ref<PipelineLayout> Layout;
        Ref<CacaoPipelineCache> Cache = nullptr;
    };
    class CACAO_API ComputePipeline : public std::enable_shared_from_this<ComputePipeline>
    {
    public:
        virtual ~ComputePipeline() = default;
        virtual Ref<PipelineLayout> GetLayout() const = 0;
    };
}
#endif 
