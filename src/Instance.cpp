#include  <Instance.h>
#include "Impls/Vulkan/VKInstance.h"
namespace Cacao
{
    Ref<Instance> Instance::Create(const InstanceCreateInfo& createInfo)
    {
        switch (createInfo.type)
        {
        case BackendType::Vulkan:
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
