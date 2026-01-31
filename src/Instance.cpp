#include  <Instance.h>
#include "Impls/Vulkan/VKInstance.h"
#include "Impls/D3D12/D3D12Instance.h"

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
        case BackendType::DirectX12:
            {
                auto inst = Cacao::CreateRef<D3D12Instance>();
                if (!inst->Initialize(createInfo))
                {
                    throw std::runtime_error("Failed to initialize D3D12 instance");
                }
                return inst;
            }
        default:
            throw std::runtime_error("Unsupported CacaoInstance type");
        }
    }
}
