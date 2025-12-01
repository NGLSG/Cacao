#ifndef CACAO_CACAOSHADERMODULE_H
#define CACAO_CACAOSHADERMODULE_H
#include <map>
namespace Cacao
{
    struct ShaderBlob
    {
        std::vector<uint8_t> Data;
        size_t Hash = 0;
    };
    using ShaderDefines = std::map<std::string, std::string>;
    struct ShaderCreateInfo
    {
        std::string SourcePath;
        std::string EntryPoint = "main";
        ShaderStage Stage = ShaderStage::Vertex;
        ShaderDefines Defines;
        std::string Profile;
    };
    class CACAO_API CacaoShaderModule : public std::enable_shared_from_this<CacaoShaderModule>
    {
    public:
        virtual ~CacaoShaderModule() = default;
        virtual const std::string& GetEntryPoint() const = 0;
        virtual ShaderStage GetStage() const = 0;
        virtual const ShaderBlob& GetBlob() const = 0;
    };
}
#endif
