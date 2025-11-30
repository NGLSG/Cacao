#include  <CacaoInstance.h>
#include "Impls/Vulkan/VKInstance.h"
namespace Cacao
{
    Ref<CacaoInstance> CacaoInstance::Create(const CacaoInstanceCreateInfo& createInfo)
    {
        switch (createInfo.type)
        {
        case CacaoType::Vulkan:
            {
                auto inst = Cacao::CreateRef<VKInstance>();
                if (!inst->Initialize(createInfo))
                {
                    throw std::runtime_error("Failed to initialize Vulkan instance");
                }
                return inst;
            }
        default:
            throw std::runtime_error("Unsupported CacaoInstance type");
        }
    }
}
