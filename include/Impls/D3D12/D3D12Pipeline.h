#ifndef CACAO_D3D12PIPELINE_H
#define CACAO_D3D12PIPELINE_H
#ifdef WIN32
#include "Pipeline.h"
#include "D3D12Common.h"

namespace Cacao
{
    class D3D12Device;
    class D3D12PipelineLayout;

    class CACAO_API D3D12PipelineCache : public PipelineCache
    {
        Ref<D3D12Device> m_device;
        ComPtr<ID3D12PipelineLibrary1> m_pipelineLibrary;
        std::vector<uint8_t> m_cacheData;

    public:
        D3D12PipelineCache(const Ref<Device>& device, std::span<const uint8_t> initialData);
        ~D3D12PipelineCache() override;
        
        std::vector<uint8_t> GetData() const override;
        void Merge(std::span<const Ref<PipelineCache>> srcCaches) override;
        
        ID3D12PipelineLibrary1* GetLibrary() const { return m_pipelineLibrary.Get(); }
        
        static Ref<PipelineCache> Create(const Ref<Device>& device, std::span<const uint8_t> initialData);
    };

    class CACAO_API D3D12GraphicsPipeline : public GraphicsPipeline
    {
        ComPtr<ID3D12PipelineState> m_pipelineState;
        Ref<D3D12Device> m_device;
        GraphicsPipelineCreateInfo m_createInfo;
        D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology;

    public:
        D3D12GraphicsPipeline(const Ref<Device>& device, const GraphicsPipelineCreateInfo& info);
        ~D3D12GraphicsPipeline() override;
        
        Ref<PipelineLayout> GetLayout() const override;
        ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
        D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_primitiveTopology; }
        
        static Ref<GraphicsPipeline> Create(const Ref<Device>& device, const GraphicsPipelineCreateInfo& info);
    };

    class CACAO_API D3D12ComputePipeline : public ComputePipeline
    {
        ComPtr<ID3D12PipelineState> m_pipelineState;
        Ref<D3D12Device> m_device;
        ComputePipelineCreateInfo m_createInfo;

    public:
        D3D12ComputePipeline(const Ref<Device>& device, const ComputePipelineCreateInfo& info);
        ~D3D12ComputePipeline() override;
        
        Ref<PipelineLayout> GetLayout() const override;
        ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
        
        static Ref<ComputePipeline> Create(const Ref<Device>& device, const ComputePipelineCreateInfo& info);
    };
}
#endif
#endif
