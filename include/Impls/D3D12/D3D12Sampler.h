#ifndef CACAO_D3D12SAMPLER_H
#define CACAO_D3D12SAMPLER_H
#ifdef WIN32
#include "Sampler.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Sampler : public Sampler
    {
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc;
    public:
        const SamplerCreateInfo& GetInfo() const override;
        D3D12_DESCRIPTOR_HEAP_DESC& GetSamplerHeapDesc()
        {
            return samplerHeapDesc;
        }
    };
};
#endif
#endif 
