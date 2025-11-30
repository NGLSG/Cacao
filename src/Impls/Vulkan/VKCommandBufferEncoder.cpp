#include "Impls/Vulkan/VKCommandBufferEncoder.h"
#include "CacaoDevice.h"
#include "CacaoTexture.h"
#include "Impls/Vulkan/VKBuffer.h"
#include "Impls/Vulkan/VKDevice.h"
#include "Impls/Vulkan/VKTexture.h"
namespace Cacao
{
    namespace
    {
        bool IsDepthFormat(Format format)
        {
            return format == Format::D32F;
        }
        bool IsStencilFormat(Format format)
        {
            return format == Format::D24S8;
        }
        vk::PipelineStageFlags TranslateStageFlags(PipelineStageFlags flags)
        {
            vk::PipelineStageFlags vkFlags;
            if (flags == PipelineStageFlags::None) return vk::PipelineStageFlagBits::eTopOfPipe; 
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::TopOfPipe))
                vkFlags |=
                    vk::PipelineStageFlagBits::eTopOfPipe;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::DrawIndirect))
                vkFlags |=
                    vk::PipelineStageFlagBits::eDrawIndirect;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::VertexInput))
                vkFlags |=
                    vk::PipelineStageFlagBits::eVertexInput;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::VertexShader))
                vkFlags |=
                    vk::PipelineStageFlagBits::eVertexShader;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::GeometryShader))
                vkFlags |=
                    vk::PipelineStageFlagBits::eGeometryShader;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::FragmentShader))
                vkFlags |=
                    vk::PipelineStageFlagBits::eFragmentShader;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::EarlyFragmentTests))
                vkFlags |=
                    vk::PipelineStageFlagBits::eEarlyFragmentTests;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::LateFragmentTests))
                vkFlags |=
                    vk::PipelineStageFlagBits::eLateFragmentTests;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::ColorAttachmentOutput))
                vkFlags
                    |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::ComputeShader))
                vkFlags |=
                    vk::PipelineStageFlagBits::eComputeShader;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::Transfer))
                vkFlags |=
                    vk::PipelineStageFlagBits::eTransfer;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::BottomOfPipe))
                vkFlags |=
                    vk::PipelineStageFlagBits::eBottomOfPipe;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::Host))
                vkFlags |=
                    vk::PipelineStageFlagBits::eHost;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::AllGraphics))
                vkFlags |=
                    vk::PipelineStageFlagBits::eAllGraphics;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(PipelineStageFlags::AllCommands))
                vkFlags |=
                    vk::PipelineStageFlagBits::eAllCommands;
            return vkFlags;
        }
        vk::AccessFlags TranslateAccessFlags(AccessFlags flags)
        {
            vk::AccessFlags vkFlags;
            if (flags == AccessFlags::None) return vkFlags;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::IndirectCommandRead))
                vkFlags |=
                    vk::AccessFlagBits::eIndirectCommandRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::IndexRead))
                vkFlags |=
                    vk::AccessFlagBits::eIndexRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::VertexAttributeRead))
                vkFlags |=
                    vk::AccessFlagBits::eVertexAttributeRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::UniformRead))
                vkFlags |=
                    vk::AccessFlagBits::eUniformRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::InputAttachmentRead))
                vkFlags |=
                    vk::AccessFlagBits::eInputAttachmentRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderRead))
                vkFlags |=
                    vk::AccessFlagBits::eShaderRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ShaderWrite))
                vkFlags |=
                    vk::AccessFlagBits::eShaderWrite;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ColorAttachmentRead))
                vkFlags |=
                    vk::AccessFlagBits::eColorAttachmentRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::ColorAttachmentWrite))
                vkFlags |=
                    vk::AccessFlagBits::eColorAttachmentWrite;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentRead))
                vkFlags
                    |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentWrite))
                vkFlags
                    |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferRead))
                vkFlags |=
                    vk::AccessFlagBits::eTransferRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::TransferWrite))
                vkFlags |=
                    vk::AccessFlagBits::eTransferWrite;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::HostRead))
                vkFlags |=
                    vk::AccessFlagBits::eHostRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::HostWrite))
                vkFlags |=
                    vk::AccessFlagBits::eHostWrite;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::MemoryRead))
                vkFlags |=
                    vk::AccessFlagBits::eMemoryRead;
            if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AccessFlags::MemoryWrite))
                vkFlags |=
                    vk::AccessFlagBits::eMemoryWrite;
            return vkFlags;
        }
        vk::ImageLayout TranslateImageLayout(ImageLayout layout)
        {
            switch (layout)
            {
            case ImageLayout::Undefined: return vk::ImageLayout::eUndefined;
            case ImageLayout::General: return vk::ImageLayout::eGeneral;
            case ImageLayout::ColorAttachment: return vk::ImageLayout::eColorAttachmentOptimal;
            case ImageLayout::DepthStencilAttachment: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case ImageLayout::DepthStencilReadOnly: return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            case ImageLayout::ShaderReadOnly: return vk::ImageLayout::eShaderReadOnlyOptimal;
            case ImageLayout::TransferSrc: return vk::ImageLayout::eTransferSrcOptimal;
            case ImageLayout::TransferDst: return vk::ImageLayout::eTransferDstOptimal;
            case ImageLayout::Present: return vk::ImageLayout::ePresentSrcKHR;
            case ImageLayout::Preinitialized: return vk::ImageLayout::ePreinitialized;
            default: return vk::ImageLayout::eUndefined;
            }
        }
        vk::ImageAspectFlags InferAspectFlags(Format format)
        {
            vk::ImageAspectFlags flags;
            bool isDepth = IsDepthFormat(format);
            bool isStencil = IsStencilFormat(format);
            if (isDepth) flags |= vk::ImageAspectFlagBits::eDepth;
            if (isStencil) flags |= vk::ImageAspectFlagBits::eStencil;
            if (!isDepth && !isStencil) flags |= vk::ImageAspectFlagBits::eColor;
            return flags;
        }
    }
    vk::CommandBufferInheritanceRenderingInfo VKCommandBufferEncoder::ConvertRenderingInfo(const RenderingInfo& info)
    {
        vk::CommandBufferInheritanceRenderingInfo vkRenderingInfo{};
        vkRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(info.ColorAttachments.size());
        std::vector<vk::Format> vkFormats(info.ColorAttachments.size(), vk::Format::eUndefined);
        for (size_t i = 0; i < info.ColorAttachments.size(); i++)
        {
            if (!info.ColorAttachments[i].ImageView)
                continue;
            switch (info.ColorAttachments[i].ImageView->GetFormat())
            {
            case Format::RGBA8_UNORM:
                vkFormats[i] = vk::Format::eR8G8B8A8Unorm;
                break;
            case Format::BGRA8_UNORM:
                vkFormats[i] = vk::Format::eB8G8R8A8Unorm;
                break;
            case Format::RGBA8_SRGB:
                vkFormats[i] = vk::Format::eR8G8B8A8Srgb;
                break;
            case Format::BGRA8_SRGB:
                vkFormats[i] = vk::Format::eB8G8R8A8Srgb;
                break;
            case Format::RGBA16_FLOAT:
                vkFormats[i] = vk::Format::eR16G16B16A16Sfloat;
                break;
            case Format::RGB10A2_UNORM:
                vkFormats[i] = vk::Format::eA2B10G10R10UnormPack32;
                break;
            case Format::RGBA32_FLOAT:
                vkFormats[i] = vk::Format::eR32G32B32A32Sfloat;
                break;
            case Format::R8_UNORM:
                vkFormats[i] = vk::Format::eR8Unorm;
                break;
            case Format::R16_FLOAT:
                vkFormats[i] = vk::Format::eR16Sfloat;
                break;
            default:
                vkFormats[i] = vk::Format::eUndefined;
                break;
            }
        }
        vkRenderingInfo.pColorAttachmentFormats = vkFormats.data();
        if (info.DepthAttachment && info.DepthAttachment->ImageView)
        {
            switch (info.DepthAttachment->ImageView->GetFormat())
            {
            case Format::D32F:
                vkRenderingInfo.depthAttachmentFormat = vk::Format::eD32Sfloat;
                break;
            case Format::D24S8:
                vkRenderingInfo.depthAttachmentFormat = vk::Format::eD24UnormS8Uint;
                break;
            default:
                vkRenderingInfo.depthAttachmentFormat = vk::Format::eUndefined;
                break;
            }
        }
        if (info.StencilAttachment && info.StencilAttachment->ImageView)
        {
            switch (info.StencilAttachment->ImageView->GetFormat())
            {
            case Format::D32F:
                vkRenderingInfo.stencilAttachmentFormat = vk::Format::eD32Sfloat;
                break;
            case Format::D24S8:
                vkRenderingInfo.stencilAttachmentFormat = vk::Format::eD24UnormS8Uint;
                break;
            default:
                vkRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;
                break;
            }
        }
        return vkRenderingInfo;
    }
    vk::RenderingInfo VKCommandBufferEncoder::ConvertRenderingInfoBegin(const RenderingInfo& info)
    {
        vk::RenderingInfo vkRenderingInfo{};
        vk::Rect2D renderArea = {
            {info.RenderArea.OffsetX, info.RenderArea.OffsetY},
            {info.RenderArea.Width, info.RenderArea.Height}
        };
        vkRenderingInfo.renderArea = renderArea;
        vkRenderingInfo.layerCount = info.LayerCount;
        m_vkColorAttachments.resize(info.ColorAttachments.size());
        for (size_t i = 0; i < info.ColorAttachments.size(); i++)
        {
            vk::RenderingAttachmentInfo& vkAttachment = m_vkColorAttachments[i];
            const RenderingAttachmentInfo& attachment = info.ColorAttachments[i];
            vkAttachment = vk::RenderingAttachmentInfo();
            if (!attachment.ImageView)
                continue;
            vkAttachment.imageView = std::dynamic_pointer_cast<VKTexture>(attachment.ImageView)->GetVulkanImageView();
            vkAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            vkAttachment.resolveMode = vk::ResolveModeFlagBits::eNone; 
            switch (attachment.LoadOp)
            {
            case AttachmentLoadOp::Load: vkAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
                break;
            case AttachmentLoadOp::Clear: vkAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                break;
            case AttachmentLoadOp::DontCare: vkAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
                break;
            }
            switch (attachment.StoreOp)
            {
            case AttachmentStoreOp::Store: vkAttachment.storeOp = vk::AttachmentStoreOp::eStore;
                break;
            case AttachmentStoreOp::DontCare: vkAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
                break;
            }
            if (attachment.LoadOp == AttachmentLoadOp::Clear)
            {
                vkAttachment.clearValue = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{
                    attachment.ClearValue.Color[0],
                    attachment.ClearValue.Color[1],
                    attachment.ClearValue.Color[2],
                    attachment.ClearValue.Color[3]
                }));
            }
        }
        vkRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(m_vkColorAttachments.size());
        vkRenderingInfo.pColorAttachments = m_vkColorAttachments.data();
        if (info.DepthAttachment && info.DepthAttachment->ImageView)
        {
            m_vkDepthAttachment = vk::RenderingAttachmentInfo();
            const RenderingAttachmentInfo& attachment = *info.DepthAttachment;
            m_vkDepthAttachment.imageView = std::dynamic_pointer_cast<VKTexture>(attachment.ImageView)->
                GetVulkanImageView();
            m_vkDepthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            m_vkDepthAttachment.resolveMode = vk::ResolveModeFlagBits::eNone;
            switch (attachment.LoadOp)
            {
            case AttachmentLoadOp::Load: m_vkDepthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
                break;
            case AttachmentLoadOp::Clear: m_vkDepthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                break;
            case AttachmentLoadOp::DontCare: m_vkDepthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
                break;
            }
            switch (attachment.StoreOp)
            {
            case AttachmentStoreOp::Store: m_vkDepthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
                break;
            case AttachmentStoreOp::DontCare: m_vkDepthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
                break;
            }
            if (attachment.LoadOp == AttachmentLoadOp::Clear)
            {
                m_vkDepthAttachment.clearValue = vk::ClearValue(vk::ClearDepthStencilValue(
                    attachment.ClearDepthStencil.Depth,
                    static_cast<uint32_t>(attachment.ClearDepthStencil.Stencil)
                ));
            }
            vkRenderingInfo.pDepthAttachment = &m_vkDepthAttachment;
        }
        else
        {
            vkRenderingInfo.pDepthAttachment = nullptr;
        }
        if (info.StencilAttachment && info.StencilAttachment->ImageView)
        {
            m_vkStencilAttachment = vk::RenderingAttachmentInfo();
            const RenderingAttachmentInfo& attachment = *info.StencilAttachment;
            m_vkStencilAttachment.imageView = std::dynamic_pointer_cast<VKTexture>(attachment.ImageView)->
                GetVulkanImageView();
            m_vkStencilAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            m_vkStencilAttachment.resolveMode = vk::ResolveModeFlagBits::eNone;
            switch (attachment.LoadOp)
            {
            case AttachmentLoadOp::Load: m_vkStencilAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
                break;
            case AttachmentLoadOp::Clear: m_vkStencilAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                break;
            case AttachmentLoadOp::DontCare: m_vkStencilAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
                break;
            }
            switch (attachment.StoreOp)
            {
            case AttachmentStoreOp::Store: m_vkStencilAttachment.storeOp = vk::AttachmentStoreOp::eStore;
                break;
            case AttachmentStoreOp::DontCare: m_vkStencilAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
                break;
            }
            if (attachment.LoadOp == AttachmentLoadOp::Clear)
            {
                m_vkStencilAttachment.clearValue = vk::ClearValue(vk::ClearDepthStencilValue(
                    attachment.ClearDepthStencil.Depth,
                    static_cast<uint32_t>(attachment.ClearDepthStencil.Stencil)
                ));
            }
            vkRenderingInfo.pStencilAttachment = &m_vkStencilAttachment;
        }
        else
        {
            vkRenderingInfo.pStencilAttachment = nullptr;
        }
        return vkRenderingInfo;
    }
    Ref<VKCommandBufferEncoder> VKCommandBufferEncoder::Create(const Ref<CacaoDevice>& device,
                                                               vk::CommandBuffer commandBuffer, CommandBufferType type)
    {
        return CreateRef<VKCommandBufferEncoder>(device, commandBuffer, type);
    }
    VKCommandBufferEncoder::VKCommandBufferEncoder(const Ref<CacaoDevice>& device, vk::CommandBuffer commandBuffer,
                                                   CommandBufferType type) :
        m_commandBuffer(commandBuffer), m_type(type)
    {
        if (!device)
        {
            throw std::runtime_error("VKCommandBufferEncoder created with null device");
        }
        m_device = std::dynamic_pointer_cast<VKDevice>(device);
    }
    void VKCommandBufferEncoder::Free()
    {
        m_device->FreeCommandBuffer(shared_from_this());
    }
    void VKCommandBufferEncoder::Reset()
    {
        m_device->ResetCommandBuffer(shared_from_this());
    }
    void VKCommandBufferEncoder::ReturnToPool()
    {
        m_device->ReturnCommandBuffer(shared_from_this());
    }
    void VKCommandBufferEncoder::Begin(const CommandBufferBeginInfo& info)
    {
        vk::CommandBufferBeginInfo beginInfo{};
        if (info.OneTimeSubmit)
            beginInfo.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        if (info.SimultaneousUse)
            beginInfo.flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        vk::CommandBufferInheritanceInfo vkInheritanceInfo;
        vk::CommandBufferInheritanceRenderingInfo vkRenderingInfo;
        if (m_type == CommandBufferType::Secondary && info.InheritanceInfo)
        {
            if (info.InheritanceInfo->pRenderingInfo)
            {
                vkRenderingInfo = ConvertRenderingInfo(*info.InheritanceInfo->pRenderingInfo);
                vkInheritanceInfo.pNext = &vkRenderingInfo;
            }
        }
        m_commandBuffer.begin(beginInfo);
    }
    void VKCommandBufferEncoder::End()
    {
        m_commandBuffer.end();
    }
    void VKCommandBufferEncoder::BeginRendering(const RenderingInfo& info)
    {
        vk::RenderingInfo renderInfo = ConvertRenderingInfoBegin(info);
        m_commandBuffer.beginRendering(renderInfo);
    }
    void VKCommandBufferEncoder::EndRendering()
    {
        m_commandBuffer.endRendering();
    }
    void VKCommandBufferEncoder::BindGraphicsPipeline(const std::shared_ptr<CacaoGraphicsPipeline>& pipeline)
    {
    }
    void VKCommandBufferEncoder::BindComputePipeline(const std::shared_ptr<CacaoComputePipeline>& pipeline)
    {
    }
    void VKCommandBufferEncoder::SetViewport(const Viewport& viewport)
    {
        m_commandBuffer.setViewport(0, vk::Viewport(
                                        viewport.X,
                                        viewport.Y,
                                        viewport.Width,
                                        viewport.Height,
                                        viewport.MinDepth,
                                        viewport.MaxDepth
                                    ));
    }
    void VKCommandBufferEncoder::SetScissor(const Rect2D& scissor)
    {
        m_commandBuffer.setScissor(0, vk::Rect2D(
                                       {scissor.OffsetX, scissor.OffsetY},
                                       {scissor.Width, scissor.Height}
                                   ));
    }
    void VKCommandBufferEncoder::BindVertexBuffer(uint32_t binding, const std::shared_ptr<CacaoBuffer>& buffer,
                                                  uint64_t offset)
    {
        m_commandBuffer.bindVertexBuffers(binding,
                                          std::dynamic_pointer_cast<VKBuffer>(buffer)->GetVulkanBuffer(),
                                          offset);
    }
    void VKCommandBufferEncoder::BindIndexBuffer(const std::shared_ptr<CacaoBuffer>& buffer, uint64_t offset,
                                                 IndexType indexType)
    {
        vk::IndexType vkIndexType;
        switch (indexType)
        {
        case IndexType::UInt16:
            vkIndexType = vk::IndexType::eUint16;
            break;
        case IndexType::UInt32:
            vkIndexType = vk::IndexType::eUint32;
            break;
        default:
            throw std::runtime_error("Unsupported index type in BindIndexBuffer");
        }
        m_commandBuffer.bindIndexBuffer(
            std::dynamic_pointer_cast<VKBuffer>(buffer)->GetVulkanBuffer(),
            offset,
            vkIndexType
        );
    }
    void VKCommandBufferEncoder::BindDescriptorSets(const std::shared_ptr<CacaoGraphicsPipeline>& pipeline,
                                                    uint32_t firstSet,
                                                    const std::vector<std::shared_ptr<CacaoDescriptorSet>>&
                                                    descriptorSets)
    {
    }
    void VKCommandBufferEncoder::PushConstants(const std::shared_ptr<CacaoGraphicsPipeline>& pipeline,
                                               ShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                               const void* data)
    {
    }
    void VKCommandBufferEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                      uint32_t firstInstance)
    {
        m_commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    void VKCommandBufferEncoder::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                             int32_t vertexOffset, uint32_t firstInstance)
    {
        m_commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    void VKCommandBufferEncoder::PipelineBarrier(PipelineStageFlags srcStage, PipelineStageFlags dstStage,
                                                 const std::vector<MemoryBarrier>& globalBarriers,
                                                 const std::vector<BufferBarrier>& bufferBarriers,
                                                 const std::vector<TextureBarrier>& textureBarriers)
    {
        vk::PipelineStageFlags vkSrcStage = TranslateStageFlags(srcStage);
        vk::PipelineStageFlags vkDstStage = TranslateStageFlags(dstStage);
        std::vector<vk::MemoryBarrier> vkGlobalBarriers;
        std::vector<vk::BufferMemoryBarrier> vkBufferBarriers;
        std::vector<vk::ImageMemoryBarrier> vkTextureBarriers;
        vkGlobalBarriers.reserve(globalBarriers.size());
        vkBufferBarriers.reserve(bufferBarriers.size());
        vkTextureBarriers.reserve(textureBarriers.size());
        for (const auto& barrier : globalBarriers)
        {
            vkGlobalBarriers.push_back(vk::MemoryBarrier()
                                       .setSrcAccessMask(TranslateAccessFlags(barrier.SrcAccess))
                                       .setDstAccessMask(TranslateAccessFlags(barrier.DstAccess))
            );
        }
        for (const auto& barrier : bufferBarriers)
        {
            auto vkBuffer = std::static_pointer_cast<VKBuffer>(barrier.Buffer);
            vkBufferBarriers.push_back(vk::BufferMemoryBarrier()
                                       .setSrcAccessMask(TranslateAccessFlags(barrier.SrcAccess))
                                       .setDstAccessMask(TranslateAccessFlags(barrier.DstAccess))
                                       .setSrcQueueFamilyIndex(barrier.SrcQueueFamilyIndex)
                                       .setDstQueueFamilyIndex(barrier.DstQueueFamilyIndex)
                                       .setBuffer(vkBuffer->GetVulkanBuffer()) 
                                       .setOffset(barrier.Offset)
                                       .setSize(barrier.Size) 
            );
        }
        for (const auto& barrier : textureBarriers)
        {
            auto vkTexture = std::static_pointer_cast<VKTexture>(barrier.Texture);
            vk::ImageAspectFlags aspectMask = InferAspectFlags(vkTexture->GetFormat());
            vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
                                                         .setAspectMask(aspectMask)
                                                         .setBaseMipLevel(barrier.SubresourceRange.BaseMipLevel)
                                                         .setLevelCount(barrier.SubresourceRange.LevelCount)
                                                         .setBaseArrayLayer(barrier.SubresourceRange.BaseArrayLayer)
                                                         .setLayerCount(barrier.SubresourceRange.LayerCount);
            vkTextureBarriers.push_back(vk::ImageMemoryBarrier()
                                        .setSrcAccessMask(TranslateAccessFlags(barrier.SrcAccess))
                                        .setDstAccessMask(TranslateAccessFlags(barrier.DstAccess))
                                        .setOldLayout(TranslateImageLayout(barrier.OldLayout))
                                        .setNewLayout(TranslateImageLayout(barrier.NewLayout))
                                        .setSrcQueueFamilyIndex(barrier.SrcQueueFamilyIndex)
                                        .setDstQueueFamilyIndex(barrier.DstQueueFamilyIndex)
                                        .setImage(vkTexture->GetVulkanImage()) 
                                        .setSubresourceRange(subresourceRange)
            );
        }
        m_commandBuffer.pipelineBarrier(
            vkSrcStage,
            vkDstStage,
            vk::DependencyFlags(), 
            static_cast<uint32_t>(vkGlobalBarriers.size()), vkGlobalBarriers.data(),
            static_cast<uint32_t>(vkBufferBarriers.size()), vkBufferBarriers.data(),
            static_cast<uint32_t>(vkTextureBarriers.size()), vkTextureBarriers.data()
        );
    }
}
