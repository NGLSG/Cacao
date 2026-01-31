#ifndef CACAO_D3D12SHADERMODULE_H
#define CACAO_D3D12SHADERMODULE_H
#ifdef WIN32
#include "D3D12Common.h"
#include "ShaderModule.h"
#include "Device.h"

namespace Cacao
{
    class D3D12Device;

    class CACAO_API D3D12ShaderModule : public ShaderModule
    {
        Ref<D3D12Device> m_device;
        ShaderBlob m_shaderBlob;
        ShaderCreateInfo m_createInfo;

    public:
        D3D12ShaderModule(
            const Ref<Device>& device,
            const ShaderCreateInfo& info,
            const ShaderBlob& blob);

        static Ref<D3D12ShaderModule> Create(
            const Ref<Device>& device,
            const ShaderCreateInfo& info,
            const ShaderBlob& blob);

        const std::string& GetEntryPoint() const override;
        ShaderStage GetStage() const override;
        const ShaderBlob& GetBlob() const override;

        // 获取 D3D12 着色器字节码描述符
        D3D12_SHADER_BYTECODE GetBytecode() const;

        ~D3D12ShaderModule() override;
    };
}
#endif
#endif