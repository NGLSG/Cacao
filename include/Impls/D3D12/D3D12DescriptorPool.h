#ifndef CACAO_D3D12DESCRIPTORPOOL_H
#define CACAO_D3D12DESCRIPTORPOOL_H
#ifdef WIN32
#include "DescriptorPool.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12DescriptorPool : public DescriptorPool
    {
        ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    public:
        ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap()
        {
            return m_descriptorHeap;
        }
        void Reset() override;
        Ref<DescriptorSet> AllocateDescriptorSet(const Ref<DescriptorSetLayout>& layout) override;
    };
}
#endif
#endif 
