#include "Impls/Vulkan/VKPipelineCache.h"
#include "Impls/Vulkan/VKDevice.h"

namespace Cacao
{
    VKPipelineCache::VKPipelineCache(const Ref<Device>& device, std::span<const uint8_t> initialData)
        : m_device(std::dynamic_pointer_cast<VKDevice>(device))
    {
        vk::PipelineCacheCreateInfo createInfo = {};
        createInfo.initialDataSize = initialData.size();
        createInfo.pInitialData = initialData.empty() ? nullptr : initialData.data();

        m_cache = m_device->GetHandle().createPipelineCache(createInfo);
    }

    VKPipelineCache::~VKPipelineCache()
    {
        if (m_device && m_cache)
        {
            m_device->GetHandle().destroyPipelineCache(m_cache);
            m_cache = nullptr;
        }
    }

    std::vector<uint8_t> VKPipelineCache::GetData() const
    {
        return m_device->GetHandle().getPipelineCacheData(m_cache);
    }

    void VKPipelineCache::Merge(std::span<const Ref<PipelineCache>> srcCaches)
    {
        std::vector<vk::PipelineCache> vkCaches;
        vkCaches.reserve(srcCaches.size());
        for (const auto& cache : srcCaches)
        {
            auto vkCache = std::dynamic_pointer_cast<VKPipelineCache>(cache);
            if (vkCache)
                vkCaches.push_back(vkCache->GetHandle());
        }

        m_device->GetHandle().mergePipelineCaches(m_cache, vkCaches);
    }
}
