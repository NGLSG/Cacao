#ifndef CACAO_D3D12PIPELINE_H
#define CACAO_D3D12PIPELINE_H
#ifdef WIN32
#include "Pipeline.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12PipelineCache : public PipelineCache
    {
    public:
        std::vector<uint8_t> GetData() const override;
        void Merge(std::span<const Ref<PipelineCache>> srcCaches) override;
    };
    class CACAO_API D3D12GraphicsPipeline : public GraphicsPipeline
    {
        ComPtr<ID3D12PipelineState> m_pipelineState;
    public:
        Ref<PipelineLayout> GetLayout() const override;
    };
    class CACAO_API D3D12ComputePipeline : public ComputePipeline
    {
        ComPtr<ID3D12PipelineState> m_pipelineState;
    public:
        Ref<PipelineLayout> GetLayout() const override;
    };
};
#endif
#endif 
