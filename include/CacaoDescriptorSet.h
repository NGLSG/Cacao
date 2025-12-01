#ifndef CACAO_CACAODESCRIPTORSET_H
#define CACAO_CACAODESCRIPTORSET_H
#include "CacaoBarrier.h"

namespace Cacao

{
    enum class DescriptorType;
    class CacaoTextureView;
    class CacaoSampler;
    class CacaoTexture;
    class CacaoBuffer;
    class CacaoDescriptorSetLayout;
    class CacaoDescriptorPool;

    struct BufferWriteInfo
    {
        uint32_t Binding = 0;
        Ref<CacaoBuffer> Buffer = nullptr;
        uint64_t Offset = 0;
        uint64_t Range = UINT64_MAX;
        DescriptorType Type;
        uint32_t ArrayElement = 0;
    };

    struct TextureWriteInfo
    {
        uint32_t Binding = 0;
        Ref<CacaoTextureView> TextureView = nullptr;
        ImageLayout Layout = ImageLayout::ShaderReadOnly;
        DescriptorType Type;
        Ref<CacaoSampler> Sampler = nullptr;
        uint32_t ArrayElement = 0;
    };

    struct AccelerationStructureWriteInfo
    {
        uint32_t Binding = 0;
        const void* AccelerationStructureHandle = nullptr;
        DescriptorType Type;
    };

    struct BufferWriteInfos
    {
        uint32_t Binding = 0;
        std::vector<Ref<CacaoBuffer>> Buffers;
        std::vector<uint64_t> Offsets;
        std::vector<uint64_t> Ranges;
        DescriptorType Type;
        uint32_t ArrayElement = 0;
    };

    struct TextureWriteInfos
    {
        uint32_t Binding = 0;
        std::vector<Ref<CacaoTextureView>> TextureViews;
        std::vector<ImageLayout> Layouts;
        DescriptorType Type;
        std::vector<Ref<CacaoSampler>> Samplers;
        uint32_t ArrayElement = 0;
    };

    struct AccelerationStructureWriteInfos
    {
        uint32_t Binding = 0;
        std::vector<const void*> AccelerationStructureHandles;
        DescriptorType Type;
        uint32_t ArrayElement = 0;
    };

    class CACAO_API CacaoDescriptorSet : public std::enable_shared_from_this<CacaoDescriptorSet>
    {
    public:
        virtual ~CacaoDescriptorSet() = default;
        virtual void WriteBuffer(const BufferWriteInfo& info) = 0;
        virtual void WriteBuffers(const BufferWriteInfos& infos) =0;
        virtual void WriteTexture(const TextureWriteInfo& info) = 0;
        virtual void WriteTextures(const TextureWriteInfos& infos) =0;
        virtual void WriteAccelerationStructure(const AccelerationStructureWriteInfo& info) = 0;
        virtual void WriteAccelerationStructures(const AccelerationStructureWriteInfos& infos) =0;
        virtual void Update() = 0;
    };
}
#endif
