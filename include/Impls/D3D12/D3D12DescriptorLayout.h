#ifndef CACAO_D3D12DESCRIPTORLAYOUT_H
#define CACAO_D3D12DESCRIPTORLAYOUT_H
#ifdef WIN32
#include "DescriptorSetLayout.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12DescriptorLayout : public DescriptorSetLayout
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
