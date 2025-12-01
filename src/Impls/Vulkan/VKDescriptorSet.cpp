#include "Impls/Vulkan/VKDescriptorSet.h"
#include "Impls/Vulkan/VKBuffer.h"
#include "Impls/Vulkan/VKDescriptorPool.h"
#include "Impls/Vulkan/VKDescriptorSetLayout.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKTexture.h"
#include "Impls/Vulkan/VKSampler.h"
namespace Cacao
{
    vk::ImageLayout Convert(ImageLayout layout)
    {
        switch (layout)
        {
        case ImageLayout::Undefined:
            return vk::ImageLayout::eUndefined;
        case ImageLayout::General:
            return vk::ImageLayout::eGeneral;
        case ImageLayout::ColorAttachment:
            return vk::ImageLayout::eColorAttachmentOptimal;
        case ImageLayout::DepthStencilAttachment:
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case ImageLayout::ShaderReadOnly:
            return vk::ImageLayout::eShaderReadOnlyOptimal;
        case ImageLayout::TransferSrc:
            return vk::ImageLayout::eTransferSrcOptimal;
        case ImageLayout::TransferDst:
            return vk::ImageLayout::eTransferDstOptimal;
        case ImageLayout::Present:
            return vk::ImageLayout::ePresentSrcKHR;
        case ImageLayout::Preinitialized:
            return vk::ImageLayout::ePreinitialized;
        default:
            return vk::ImageLayout::eUndefined;
        }
    }
    vk::DescriptorType GetBufferUsage(const BufferUsageFlags& usage)
    {
        vk::DescriptorType usageFlags;
        if (usage & BufferUsageFlags::VertexBuffer)
        {
            usageFlags = vk::DescriptorType::eStorageBuffer;
        }
        else if (usage & BufferUsageFlags::IndexBuffer)
        {
            usageFlags = vk::DescriptorType::eStorageBuffer;
        }
        else if (usage & BufferUsageFlags::UniformBuffer)
        {
            usageFlags = vk::DescriptorType::eUniformBuffer;
        }
        else if (usage & BufferUsageFlags::StorageBuffer)
        {
            usageFlags = vk::DescriptorType::eStorageBuffer;
        }
        else
        {
            throw std::runtime_error("Unsupported BufferUsageFlags in GetBufferUsage");
        }
        return usageFlags;
    }
    vk::DescriptorType Convert(DescriptorType type)
    {
        switch (type)
        {
        case DescriptorType::Sampler:
            return vk::DescriptorType::eSampler;
        case DescriptorType::CombinedImageSampler:
            return vk::DescriptorType::eCombinedImageSampler;
        case DescriptorType::SampledImage:
            return vk::DescriptorType::eSampledImage;
        case DescriptorType::StorageImage:
            return vk::DescriptorType::eStorageImage;
        case DescriptorType::UniformBuffer:
            return vk::DescriptorType::eUniformBuffer;
        case DescriptorType::StorageBuffer:
            return vk::DescriptorType::eStorageBuffer;
        case DescriptorType::UniformBufferDynamic:
            return vk::DescriptorType::eUniformBufferDynamic;
        case DescriptorType::StorageBufferDynamic:
            return vk::DescriptorType::eStorageBufferDynamic;
        case DescriptorType::InputAttachment:
            return vk::DescriptorType::eInputAttachment;
        case DescriptorType::AccelerationStructure:
            return vk::DescriptorType::eAccelerationStructureKHR;
        default:
            return vk::DescriptorType::eSampler;
        }
    }
    vk::DescriptorBufferInfo VKDescriptorSet::ConvertToVkBufferInfo(const BufferWriteInfo& info)
    {
        vk::DescriptorBufferInfo vkInfo = {};
        auto vkBuffer = std::static_pointer_cast<VKBuffer>(info.Buffer);
        vkInfo.buffer = vkBuffer->GetHandle();
        vkInfo.offset = info.Offset;
        vkInfo.range = info.Range == UINT64_MAX ? vkBuffer->GetSize() - info.Offset : info.Range;
        return vkInfo;
    }
    vk::DescriptorImageInfo VKDescriptorSet::ConvertToVkImageInfo(const TextureWriteInfo& info)
    {
        vk::DescriptorImageInfo vkInfo = {};
        if (!info.TextureView)
        {
            throw std::runtime_error("TextureWriteInfo has null Texture");
        }
        if (info.Sampler)
        {
            auto vkSampler = std::static_pointer_cast<VKSampler>(info.Sampler);
            vkInfo.sampler = vkSampler->GetHandle();
        }
        auto vkView = std::static_pointer_cast<VKTextureView>(info.TextureView);
        vkInfo.imageView = vkView->GetHandle();
        vkInfo.imageLayout = Convert(info.Layout);
        return vkInfo;
    }
    VKDescriptorSet::VKDescriptorSet(const Ref<CacaoDevice>& device, const Ref<CacaoDescriptorPool>& parent,
                                     const Ref<CacaoDescriptorSetLayout>& layout,
                                     const vk::DescriptorSet& descriptorSet)
    {
        if (!device)
        {
            throw std::runtime_error("VKDescriptorSet created with null device");
        }
        m_device = std::static_pointer_cast<VKDevice>(device);
        if (!parent)
        {
            throw std::runtime_error("VKDescriptorSet created with null parent pool");
        }
        if (!layout)
        {
            throw std::runtime_error("VKDescriptorSet created with null layout");
        }
        m_parentPool = std::static_pointer_cast<VKDescriptorPool>(parent);
        m_layout = std::static_pointer_cast<VKDescriptorSetLayout>(layout);
        m_descriptorSet = descriptorSet;
        if (!m_descriptorSet)
        {
            throw std::runtime_error("VKDescriptorSet created with invalid vk::DescriptorSet");
        }
    }
    Ref<VKDescriptorSet> VKDescriptorSet::Create(const Ref<CacaoDevice>& device, const Ref<CacaoDescriptorPool>& parent,
                                                 const Ref<CacaoDescriptorSetLayout>& layout,
                                                 const vk::DescriptorSet& descriptorSet)
    {
        return CreateRef<VKDescriptorSet>(device, parent, layout, descriptorSet);
    }
    void VKDescriptorSet::Update()
    {
        if (!m_pendingWrites.empty())
        {
            m_device->GetHandle().updateDescriptorSets(
                static_cast<uint32_t>(m_pendingWrites.size()),
                m_pendingWrites.data(),
                0,
                nullptr);
            m_pendingWrites.clear();
            m_pendingBufferInfos.clear();
            m_pendingImageInfos.clear();
            m_pendingASInfos.clear();
            m_pendingASHandles.clear();
            m_pendingBufferInfoArrays.clear();
            m_pendingImageInfoArrays.clear();
            m_pendingASHandleArrays.clear();
        }
    }
    void VKDescriptorSet::WriteBuffer(const BufferWriteInfo& info)
    {
        m_pendingBufferInfos.push_back(ConvertToVkBufferInfo(info));
        vk::DescriptorBufferInfo* pBufferInfo = &m_pendingBufferInfos.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = info.Binding;
        vkWriteDescriptorSet.dstArrayElement = info.ArrayElement;
        vkWriteDescriptorSet.descriptorType = Convert(info.Type);
        vkWriteDescriptorSet.descriptorCount = 1;
        vkWriteDescriptorSet.pBufferInfo = pBufferInfo;
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
    void VKDescriptorSet::WriteTexture(const TextureWriteInfo& info)
    {
        m_pendingImageInfos.push_back(ConvertToVkImageInfo(info));
        vk::DescriptorImageInfo* pImageInfo = &m_pendingImageInfos.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = info.Binding;
        vkWriteDescriptorSet.dstArrayElement = info.ArrayElement;
        vkWriteDescriptorSet.descriptorType = Convert(info.Type);
        vkWriteDescriptorSet.descriptorCount = 1;
        vkWriteDescriptorSet.pImageInfo = pImageInfo;
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
    void VKDescriptorSet::WriteAccelerationStructure(const AccelerationStructureWriteInfo& info)
    {
        m_pendingASHandles.push_back(
            *reinterpret_cast<const vk::AccelerationStructureKHR*>(&info.AccelerationStructureHandle));
        vk::AccelerationStructureKHR* pASHandle = &m_pendingASHandles.back();
        vk::WriteDescriptorSetAccelerationStructureKHR vkASInfo = {};
        vkASInfo.accelerationStructureCount = 1;
        vkASInfo.pAccelerationStructures = pASHandle;
        m_pendingASInfos.push_back(vkASInfo);
        vk::WriteDescriptorSetAccelerationStructureKHR* pASInfo = &m_pendingASInfos.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = info.Binding;
        vkWriteDescriptorSet.dstArrayElement = 0;
        vkWriteDescriptorSet.descriptorType = Convert(info.Type);
        vkWriteDescriptorSet.descriptorCount = 1;
        vkWriteDescriptorSet.pNext = pASInfo;
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
    void VKDescriptorSet::WriteBuffers(const BufferWriteInfos& infos)
    {
        std::vector<vk::DescriptorBufferInfo> vkBufferInfos;
        vkBufferInfos.resize(infos.Buffers.size());
        for (size_t i = 0; i < infos.Buffers.size(); ++i)
        {
            vkBufferInfos[i] = ConvertToVkBufferInfo(
                BufferWriteInfo{
                    infos.Binding,
                    infos.Buffers[i],
                    infos.Offsets.size() > i ? infos.Offsets[i] : 0,
                    infos.Ranges.size() > i ? infos.Ranges[i] : UINT64_MAX,
                    infos.Type,
                    infos.ArrayElement + static_cast<uint32_t>(i)
                });
        }
        m_pendingBufferInfoArrays.push_back(std::move(vkBufferInfos));
        std::vector<vk::DescriptorBufferInfo>* pBufferInfoArray = &m_pendingBufferInfoArrays.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = infos.Binding;
        vkWriteDescriptorSet.dstArrayElement = infos.ArrayElement;
        vkWriteDescriptorSet.descriptorType = Convert(infos.Type);
        vkWriteDescriptorSet.descriptorCount = static_cast<uint32_t>(pBufferInfoArray->size());
        vkWriteDescriptorSet.pBufferInfo = pBufferInfoArray->data();
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
    void VKDescriptorSet::WriteTextures(const TextureWriteInfos& infos)
    {
        std::vector<vk::DescriptorImageInfo> vkImageInfos;
        vkImageInfos.resize(infos.TextureViews.size());
        for (size_t i = 0; i < infos.TextureViews.size(); ++i)
        {
            vkImageInfos[i] = ConvertToVkImageInfo(
                TextureWriteInfo{
                    infos.Binding,
                    infos.TextureViews[i],
                    infos.Layouts.size() > i ? infos.Layouts[i] : ImageLayout::ShaderReadOnly,
                    infos.Type,
                    infos.Samplers.size() > i ? infos.Samplers[i] : nullptr,
                    infos.ArrayElement + static_cast<uint32_t>(i)
                });
        }
        m_pendingImageInfoArrays.push_back(std::move(vkImageInfos));
        std::vector<vk::DescriptorImageInfo>* pImageInfoArray = &m_pendingImageInfoArrays.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = infos.Binding;
        vkWriteDescriptorSet.dstArrayElement = infos.ArrayElement;
        vkWriteDescriptorSet.descriptorType = Convert(infos.Type);
        vkWriteDescriptorSet.descriptorCount = static_cast<uint32_t>(pImageInfoArray->size());
        vkWriteDescriptorSet.pImageInfo = pImageInfoArray->data();
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
    void VKDescriptorSet::WriteAccelerationStructures(const AccelerationStructureWriteInfos& infos)
    {
        std::vector<vk::AccelerationStructureKHR> vkASHandles;
        vkASHandles.resize(infos.AccelerationStructureHandles.size());
        for (size_t i = 0; i < infos.AccelerationStructureHandles.size(); ++i)
        {
            vkASHandles[i] = *static_cast<const vk::AccelerationStructureKHR*>(
                infos.AccelerationStructureHandles[i]);
        }
        m_pendingASHandleArrays.push_back(std::move(vkASHandles));
        std::vector<vk::AccelerationStructureKHR>* pASHandleArray = &m_pendingASHandleArrays.back();
        vk::WriteDescriptorSetAccelerationStructureKHR vkASInfo = {};
        vkASInfo.accelerationStructureCount = static_cast<uint32_t>(pASHandleArray->size());
        vkASInfo.pAccelerationStructures = pASHandleArray->data();
        m_pendingASInfos.push_back(vkASInfo);
        vk::WriteDescriptorSetAccelerationStructureKHR* pASInfo = &m_pendingASInfos.back();
        vk::WriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.dstSet = m_descriptorSet;
        vkWriteDescriptorSet.dstBinding = infos.Binding;
        vkWriteDescriptorSet.dstArrayElement = infos.ArrayElement;
        vkWriteDescriptorSet.descriptorType = Convert(infos.Type);
        vkWriteDescriptorSet.descriptorCount = static_cast<uint32_t>(pASHandleArray->size());
        vkWriteDescriptorSet.pNext = pASInfo;
        m_pendingWrites.push_back(vkWriteDescriptorSet);
    }
}
