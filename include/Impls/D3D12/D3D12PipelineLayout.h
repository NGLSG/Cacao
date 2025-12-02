#ifndef CACAO_D3D12PIPELINELAYOUT_H
#define CACAO_D3D12PIPELINELAYOUT_H
#ifdef WIN32
#include "D3D12Common.h"
#include "PipelineLayout.h"
namespace Cacao
{
    class CACAO_API D3D12PipelineLayout : public PipelineLayout
    {
        ComPtr<ID3D12RootSignature> m_rootSignature;
    public:
        ComPtr<ID3D12RootSignature>& GetRootSignature()
        {
            return m_rootSignature;
        }
    };
}
#endif
#endif 
