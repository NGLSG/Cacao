#ifndef CACAO_CACAODESCRIPTORPOOL_H
#define CACAO_CACAODESCRIPTORPOOL_H
#include "CacaoDescriptorSetLayout.h"
namespace Cacao
{
    class CacaoDescriptorSet;
    struct DescriptorPoolSize
    {
        DescriptorType Type;
        uint32_t Count; 
    };
    struct DescriptorPoolCreateInfo
    {
        uint32_t MaxSets = 100; 
        std::vector<DescriptorPoolSize> PoolSizes;
    };
    class CACAO_API CacaoDescriptorPool : public std::enable_shared_from_this<CacaoDescriptorPool>
    {
    public:
        virtual ~CacaoDescriptorPool() = default;
        virtual void Reset() = 0;
        virtual Ref<CacaoDescriptorSet> AllocateDescriptorSet(
            const Ref<CacaoDescriptorSetLayout>& layout) = 0;
    };
}
#endif 
