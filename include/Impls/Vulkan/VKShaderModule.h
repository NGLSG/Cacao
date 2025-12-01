#ifndef CACAO_VKSHADERMODULE_H
#define CACAO_VKSHADERMODULE_H
#include "CacaoShaderModule.h"
#include <vulkan/vulkan.hpp>
namespace Cacao
{
    class CacaoDevice;
    class VKDevice;
    class CACAO_API VKShaderModule : public CacaoShaderModule
    {
        Ref<VKDevice> m_device;
        vk::ShaderModule m_module;
        ShaderBlob m_shaderBlob;
        ShaderCreateInfo m_createInfo;
    public:
        VKShaderModule(
            const Ref<CacaoDevice>& device,
            const ShaderCreateInfo& info,
            const ShaderBlob& blob);
        static Ref<VKShaderModule> Create(
            const Ref<CacaoDevice>& device,
            const ShaderCreateInfo& info,
            const ShaderBlob& blob);
        const std::string& GetEntryPoint() const override;
        ShaderStage GetStage() const override;
        const ShaderBlob& GetBlob() const override;
        vk::ShaderModule& GetHandle()
        {
            return m_module;
        }
        ~VKShaderModule() override;
    };
}
#endif 
