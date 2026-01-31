#ifndef CACAO_D3D12PIPELINELAYOUT_H
#define CACAO_D3D12PIPELINELAYOUT_H
#ifdef WIN32
#include "D3D12Common.h"
#include "PipelineLayout.h"

namespace Cacao
{
    class D3D12Device;

    class CACAO_API D3D12PipelineLayout : public PipelineLayout
    {
        ComPtr<ID3D12RootSignature> m_rootSignature;
        Ref<D3D12Device> m_device;
        PipelineLayoutCreateInfo m_createInfo;

        // 存储每个描述符集对应的根参数索引范围
        std::vector<std::pair<uint32_t, uint32_t>> m_setRootParamRange;
        // Push constants 根参数索引
        uint32_t m_pushConstantsRootIndex = UINT32_MAX;

    public:
        ComPtr<ID3D12RootSignature>& GetRootSignature() { return m_rootSignature; }
        const PipelineLayoutCreateInfo& GetCreateInfo() const { return m_createInfo; }

        // 获取描述符集对应的根参数起始索引
        uint32_t GetSetRootParamStartIndex(uint32_t setIndex) const;
        uint32_t GetPushConstantsRootIndex() const { return m_pushConstantsRootIndex; }

        static Ref<PipelineLayout> Create(const Ref<Device>& device, const PipelineLayoutCreateInfo& info);
        D3D12PipelineLayout(const Ref<Device>& device, const PipelineLayoutCreateInfo& info);
        ~D3D12PipelineLayout() override;
    };
}
#endif
#endif
