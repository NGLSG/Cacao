#ifndef CACAO_D3D12SHADERMODULE_H
#define CACAO_D3D12SHADERMODULE_H
#ifdef WIN32
#include "D3D12Common.h"
#include "ShaderModule.h"
namespace Cacao
{
    class CACAO_API D3D12ShaderModule: public ShaderModule
    {
    public:
        const std::string& GetEntryPoint() const override;
        ShaderStage GetStage() const override;
        const ShaderBlob& GetBlob() const override;
    private:
        ComPtr<ID3DBlob> m_shaderBlob;
    };
}
#endif
#endif 
