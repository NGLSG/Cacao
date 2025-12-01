#ifndef CACAO_CACAODESCRIPTOR_H
#define CACAO_CACAODESCRIPTOR_H
#include <vector>
#include <memory>
namespace Cacao
{
    class CacaoBuffer;
    class CacaoTexture;
    class CacaoSampler;
    enum class DescriptorType
    {
        Sampler, 
        CombinedImageSampler, 
        SampledImage, 
        StorageImage, 
        UniformBuffer, 
        StorageBuffer, 
        UniformBufferDynamic, 
        StorageBufferDynamic, 
        InputAttachment, 
        AccelerationStructure 
    };
    struct DescriptorSetLayoutBinding
    {
        uint32_t Binding = 0; 
        DescriptorType Type = DescriptorType::UniformBuffer;
        uint32_t Count = 1; 
        ShaderStage StageFlags = ShaderStage::AllGraphics; 
        std::vector<std::shared_ptr<CacaoSampler>> ImmutableSamplers;
    };
    struct DescriptorSetLayoutCreateInfo
    {
        std::vector<DescriptorSetLayoutBinding> Bindings;
        bool SupportBindless = false; 
    };
    class CACAO_API CacaoDescriptorSetLayout : public std::enable_shared_from_this<CacaoDescriptorSetLayout>
    {
    public:
        virtual ~CacaoDescriptorSetLayout() = default;
    };
}
#endif 
