#ifndef CACAO_CACAOPIPELINELAYOUT_H
#define CACAO_CACAOPIPELINELAYOUT_H
#include "PipelineDefs.h"
#include <memory>
#include <vector>
namespace Cacao
{
    class CacaoDescriptorSetLayout;
    struct PipelineLayoutCreateInfo
    {
        std::vector<Ref<CacaoDescriptorSetLayout>> SetLayouts;
        std::vector<PushConstantRange> PushConstantRanges;
    };
    class CACAO_API CacaoPipelineLayout : public std::enable_shared_from_this<CacaoPipelineLayout>
    {
    public:
        virtual ~CacaoPipelineLayout() = default;
    };
}
#endif 
